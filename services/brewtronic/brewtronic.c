/*
 * Copyright (c) 2009 by Stefan Riepenhausen <rhn@gmx.net>
 *
 */

#define VERSION "rkBREW 0.44"

#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include "core/tty/tty.h"
#include "core/debug.h"
#include "core/eeprom.h"
#include "core/bit-macros.h"

#include "services/clock/clock.h"
#include "hardware/onewire/onewire.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "appsample.h"

#include "protocols/ecmd/ecmd-base.h"

// Encoder 
#define PHASE_A     (PINC & 1<<PC3)
#define PHASE_B     (PINC & 1<<PC4)
#define PUSH_ENC    (PINC & 1<<PC5)
// LED
#define HEAT       PC0 
#define BLUELED    PC1
#define REDLED     PC2
// BEEP
#define BEEP PA7

static uint16_t cur_time;

// Status Variablen
static int last_sec = 0;
static int last_min = 0;
static int phase = 0;
static uint8_t start_proc = 0;
static uint8_t brew_step = 255;
static uint16_t  progess=0;
struct clock_datetime_t cur_date;

static uint8_t rast_temp[MAX_BREW];
static uint8_t rast_zeit[MAX_BREW];
static uint8_t t_hist[MAX_HIST];
static uint8_t hist_cnt = 0;
static uint8_t hist_saved = 0;
static uint16_t cur_temp;


volatile int8_t enc_delta;          // -128 ... 127
static int8_t last;
static int8_t started = 0;


uint8_t ready = 0; // Fertig
uint8_t process_pause = 0;  // Programmierte Pause
WINDOW *upper ;
WINDOW *progress ;
WINDOW *lower ;

uint8_t up_heating = 1;
uint8_t heat_time = 0;

void reg_temp (uint8_t perc) {
  if (brew_step == 255) { // start
    brew_step = 0;
    cur_time = 0;
    if (cur_temp < rast_temp[brew_step]*2) 
      up_heating = 1;
    else
      up_heating = 0;
  }
  if (cur_temp < rast_temp[brew_step]*2) {
    if (heat_time < perc) PORTC |= _BV(HEAT);
    heat_time = (heat_time+1) % 10;
    
  } else {
    if (up_heating) {
      up_heating = 0;     // temp reached
      cur_time = 0;	// starte Rast
    };
    heat_time = 0;
    PORTC &= ~ _BV(HEAT);
  }
  if (cur_time >= rast_zeit[brew_step]) {
    brew_step++;
    cur_time = 0;
    up_heating = 1;
    if (rast_zeit[brew_step] == 0 || rast_temp[brew_step] > 100) {
      start_proc = 0;  // ready! 
      ready = 1;
    }
  }
}

// Dekodertabelle für wackeligen Rastpunkt
// halbe Auflösung
//int8_t table[16] PROGMEM = {0,0,-1,0,0,0,0,1,1,0,0,0,0,-1,0,0};     
// Dekodertabelle für normale Drehgeber
// volle Auflösung
int8_t table[16] PROGMEM = {0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0};    
 
// struct ow_rom_code_t rom;
/*
  If enabled in menuconfig, this function is called during boot up of ethersex
*/

// 0..100
void set_progress (int16_t percent)  {
  uint16_t stars;
  uint16_t spaces;
  uint8_t pausing = start_proc;
  percent = percent<0 ? 0 : percent;
  percent = percent>100 ? 100 : percent;
  stars = 14*percent/100;
  stars = stars>14 ? 14 : stars;
  spaces = 14-stars;
  wclear(progress);
  if (pausing == 0)
     wprintw(progress,"{");
  else
     wprintw(progress,"[");
  if (stars > 0) {
    while (stars--) {
      wprintw(progress,":");
    }
  }
  if (spaces > 0) {
    while (spaces--) {
      wprintw(progress," ");
    }
  }
  if (pausing == 0)
     wprintw(progress,"}");
  else
     wprintw(progress,"]");
}

uint8_t boot_wait ;
int16_t
app_sample_init(void)
{
  boot_wait = 10;
  APPSAMPLEDEBUG ("init\n");
  int16_t ret;
  hist_saved=0;
  hist_cnt=0;
  upper = subwin(curscr,1,16,0,0);
  progress = subwin(curscr,1,16,1,0);
  lower = subwin(curscr,2,16,2,0);
  clear();
  wprintw(upper,VERSION);
  wprintw(lower,__DATE__);
  wprintw(lower,"\nStart...\n");
  set_progress(0);
  eeprom_restore(rast_temp_ee,rast_temp,MAX_BREW);
  eeprom_restore(rast_zeit_ee,rast_zeit,MAX_BREW);
  eeprom_restore(t_hist_ee,t_hist,MAX_HIST);
  DDRC  =(uint8_t) ~  0xF8; // PC0..2 Outputs
  DDRA  = 0x80; // PA7 Output
  PORTA = 0xff;
  PORTC = 0xF8; // PULLUP

  /* disable interrupts */
  uint8_t sreg = SREG;
  cli();
  ret = ow_search_rom_first();
  /* re-enable interrupts */
  SREG = sreg;

  if (ret <= 0) {
    return ECMD_FINAL_OK;
  }
  cur_time = 0;
  return ECMD_FINAL_OK;
}

void BlueLed(char on)
{
  if (on == 1) {
    PORTC |= _BV(BLUELED);
  } else {
    PORTC &= ~_BV(BLUELED);    
  }
}

/*
  Run each  20 ms
*/
 uint8_t press_time = 0;
 uint8_t release_time = 0;
 uint8_t beep_time = 0;
int16_t
app_sample_periodic(void)
{
  uint8_t i;
  int16_t ret;
  int16_t prog_state=0;
  uint8_t sreg ;
  uint8_t rast = brew_step==255?0:brew_step ;
  struct ow_temp_scratchpad_t sp;
  clock_current_localtime(&cur_date);
  // Update beeper
  if (beep_time > 0) {
    beep_time--;
    PORTA &= ~_BV(BEEP);
  } else
    PORTA |= _BV(BEEP);

  // update rotary
  last = (last << 2)  & 0x0F;
  if (PHASE_A) last |=2;
  if (PHASE_B) last |=1;
  enc_delta += pgm_read_byte(&table[last]);

  // update button
  if (PUSH_ENC != 0 && release_time > 0)   release_time--;

  if (PUSH_ENC == 0) {
    if (press_time++ == 1 ) {
      beep_time = 5;
      start_proc = ! start_proc;
      if (started==0) {  // first start
	started++;
	for (i=0;i<MAX_HIST;i++) {
	  t_hist[i] = 255;
	}
      }
    }
  }
 else  {
    release_time = 3;
    press_time = 0;
 }
  // update LED
  BlueLed(start_proc);

  // once a second
  if (cur_date.sec != last_sec) {
    if (boot_wait>0) boot_wait--;
    if  (boot_wait == 0 ) { 
      last_sec = cur_date.sec;
      if (start_proc) {
	reg_temp(8); 
      } else
	PORTC &= ~_BV(HEAT);
      phase++;
      phase &= 7; // 3 bits
      switch (phase) {
      case 0: 
	if (ready == 1) {
	  wprintw(lower,"Fertig! %2d\n",rast);
	}
	break;
      case 1:
	if (rast == 0)
	  wprintw(lower,"Einmaischen |%2d\n",up_heating);	  
	else 
	  wprintw(lower,"Rast: %2d  %2d|%2d\n",
		  rast,rast_zeit[brew_step],up_heating);	  
//      case 5: 
      case 3:
	wclear(upper);
	wprintw(upper,"%2d %2d|%2d %2d|%2d.%1d",rast,cur_time,
		rast_zeit[brew_step]==255?0:rast_zeit[brew_step],
		rast_temp[brew_step]==255?0:rast_temp[brew_step],
		cur_temp/2,cur_temp&1?5:0 );
	prog_state = (cur_time*100)/rast_zeit[brew_step];
	set_progress(prog_state);
	if (ready == 1 && hist_saved==0) {
	  hist_saved = 1;
	  t_hist[hist_cnt<MAX_HIST-1?hist_cnt++:hist_cnt] = 255 ; // EOF
	  eeprom_save(t_hist_ee,t_hist,hist_cnt);
	}
	break;
      case 6:
	if (start_proc)
	  wprintw(lower,"Run  Rast: %2d\n",rast);	  
	else
	  wprintw(lower,"Stop Rast: %2d\n",rast);	  
	if (PINC & _BV(HEAT))
	  wprintw(lower,"Heizen ein\n");	  
	else
	  wprintw(lower,"Heizen aus\n");	
	break;
	// Convert 
      case 4:
//      case 2:
      case 7:
	sreg = SREG;
	cli();
	ret = ow_temp_start_convert_wait(&ow_global.current_rom);
	ret = ow_temp_read_scratchpad(&ow_global.current_rom, &sp);
	/* re-enable interrupts */
	SREG = sreg;
	cur_temp  = ow_temp_normalize(&ow_global.current_rom, &sp);
	cur_temp = HI8(cur_temp)*2  + (LO8(cur_temp)>127?1:0);
	break;
      default:
 	break;
      }
    }
  }
  // each minute
  if (cur_date.min != last_min ) {
    last_min = cur_date.min;
    if (!up_heating && start_proc) // running and not heating up
      cur_time++;	
    if (start_proc && hist_cnt<MAX_HIST)
      t_hist[hist_cnt++] = cur_temp;
  }
  return ECMD_FINAL_OK;
}
//
// Commands 
//
int16_t
brew_start(uint8_t start_stop) {
  APPSAMPLEDEBUG ("set\n");
  if (start_stop == 0) {
    start_proc = 1;
  } else {
    start_proc = 0;
  }
    return ECMD_FINAL_OK; 
}

char temp_str[10];
char * TempToStr(uint16_t temp) {
  snprintf(temp_str,9,"%3u.%1u",temp/2,temp&1?5:0);
  return temp_str;
}

int16_t
brew_temp(char *cmd, char *output, uint16_t len) {
  return ECMD_FINAL(snprintf_P(output, len, PSTR("%s"),TempToStr(cur_temp)));  
}

int16_t
brew_set(uint8_t step, uint8_t temp, uint8_t mins) {
  APPSAMPLEDEBUG ("set\n");
  wprintw(upper,"Set %d",step);
  if (step < MAX_BREW) {
    rast_zeit[step] = mins;
    rast_temp[step] = temp;
  }
  return ECMD_FINAL_OK; 
}

int16_t
brew_save(char *cmd, char *output, uint16_t len){
  APPSAMPLEDEBUG ("save\n");
  wprintw(upper,"Save");
   BlueLed(1);
  eeprom_save(rast_temp_ee,rast_temp,MAX_BREW);
  eeprom_save(rast_zeit_ee,rast_zeit,MAX_BREW);
  eeprom_save(t_hist_ee,t_hist,MAX_HIST);
  //return ECMD_FINAL(snprintf_P(output, len, PSTR("rast_zeit:  %u"),rast_zeit[0]));

  return ECMD_FINAL_OK;
}

int16_t
brew_restore(char *cmd, char *output, uint16_t len){
  APPSAMPLEDEBUG ("restore\n");
  wprintw(upper,"Restore");
  eeprom_restore(rast_temp_ee,rast_temp,MAX_BREW);
  eeprom_restore(rast_zeit_ee,rast_zeit,MAX_BREW);
  eeprom_restore(t_hist_ee,t_hist,MAX_HIST);
  //  return ECMD_FINAL(snprintf_P(output, len, PSTR("tim:  %u"),rast_temp[0]));

    return ECMD_FINAL_OK;
}

static int hist_restore_cnt=0;
int16_t
brew_history(char *cmd, char *output, uint16_t len) {
   BlueLed(1);
   if (t_hist[hist_restore_cnt] != 255 && hist_restore_cnt < MAX_HIST) {
     return ECMD_AGAIN(snprintf_P(output, len, PSTR("%d,%s"),hist_restore_cnt,TempToStr(t_hist[hist_restore_cnt++])));
    } else {
     hist_restore_cnt = 0;
     return ECMD_FINAL_OK;
   }	
}

/*
  -- Ethersex META --
  header(services/appsample/appsample.h)
  ifdef(`conf_APP_SAMPLE_INIT_AUTOSTART',`init(app_sample_init)')
  ifdef(`conf_APP_SAMPLE_PERIODIC_AUTOSTART',`timer(2,app_sample_periodic())')
*/

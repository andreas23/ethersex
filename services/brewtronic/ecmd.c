/*
 * Copyright (c) 2009 by Stefan Riepenhausen <rhn@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For more information on the GPL, please go to:
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <avr/io.h>
#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "config.h"
#include "appsample.h"
#include "protocols/ecmd/ecmd-base.h"

int16_t parse_cmd_brew_set_command(char *cmd, char *output, uint16_t len) 
{
    uint8_t step,temp,tim;
    while (*cmd == ' ')
	cmd++;
	if (cmd[0]!=0) 
		sscanf_P(cmd, PSTR("%hu %hu %hu"),  &step,&temp,&tim);
	
  return brew_set(step, temp, tim);
}

int16_t parse_cmd_brew_save_command(char *cmd, char *output, uint16_t len) 
{
  return brew_save(cmd,output,len);
}


int16_t parse_cmd_brew_temp_command(char *cmd, char *output, uint16_t len) 
{
  return brew_temp(cmd,output,len);
}

int16_t parse_cmd_brew_restore_command(char *cmd, char *output, uint16_t len) 
{
  return brew_restore(cmd,output,len);
}

int16_t parse_cmd_brew_hist_command(char *cmd, char *output, uint16_t len) 
{
  return brew_history(cmd,output,len);
}

int16_t parse_cmd_app_sample_init(char *cmd, char *output, uint16_t len) 
{
  return app_sample_init();
}

int16_t parse_cmd_app_sample_periodic(char *cmd, char *output, uint16_t len) 
{
  return app_sample_periodic();
}

int16_t parse_cmd_brew_start_command(char *cmd, char *output, uint16_t len) 
{
    uint8_t step;
    while (*cmd == ' ')
	cmd++;
	if (cmd[0]!=0) 
		sscanf_P(cmd, PSTR("%hu"),  &step);
	
  return brew_start(step);

}

/*
-- Ethersex META --
block([[Application_Sample]])
ecmd_feature(brew_set_command, "brewset",[step] [temp] [time], Set rast)
ecmd_feature(brew_restore_command, "brewrest",, restore)
ecmd_feature(brew_save_command, "brewsave",, save)
ecmd_feature(brew_start_command, "brewstart",, save)
ecmd_feature(brew_temp_command, "brewtemp",, save)
ecmd_feature(brew_hist_command, "brewhist",, save)
ecmd_feature(app_sample_init, "brewinit",, Manually call application sample init method)
ecmd_feature(app_sample_periodic, "brewperiodic",, Manually call application sample periodic method)
*/

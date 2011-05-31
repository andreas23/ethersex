#!/usr/bin/perl -w
  #Auslesen der 1 Wire Sensoren an einem AVR-NET-IO mit ethersex
 
  use strict;
  use Net::Telnet ();
 
  my $esexip="192.168.115.99";
  my $esexport="2701";
  my $esex;
  my @sensor;
  my $sensor;
  my $dummy;
  my $temp;
 
  $esex = Net::Telnet->new || die "kann Ethersex nicht finden";;
  $esex->open(Host  => $esexip,
            Port    => $esexport,
            Timeout => 2);
 
  #Alles Sensor-IDs auslesen und dem Array @sensor zuweisen
  $esex->print("1w list");
  ($sensor) = $esex->waitfor(Timeout => 2,
                             String  => "OK");
  @sensor=split(/\s+/, $sensor);
        print "@sensor","\n"; #Kontrollausgabe
 
  my $zahler=@sensor;
  print "Anzahl der Elemente :",$zahler,"\n\n";
 
  #Alles Sensore Temperatur einlesen
  $esex->print("1w convert");
  $esex->waitfor(Timeout => 2,
                 String  => "OK");
 
  #Sensor ID inklusive Wert ausgeben
  foreach (@sensor) {
        $esex->print("1w get $_");
 
  ($dummy,$temp)=$esex->waitfor(Match   =>'/[-]?\d+\.\d+/',
                                Timeout => 5);
 
   print "Temperatur vom ID ",$_,": ",$temp," C°","\n";
  $esex->print("hostname");

   my $hostn;
  ($dummy,$hostn)=$esex->waitfor( Timeout => 5, Match =>'/[0-9A-Za-z]+/');
  print $hostn," $dummy";

  $esex->print("lcd clear");
  $esex->waitfor(Timeout => 2, String  => "OK");
  $esex->print("lcd write Hello World! $temp");
 $esex->waitfor(Timeout => 2, String  => "OK");
  }

#!/usr/bin/perl -w

 
  use strict;
  use Net::Telnet ();
 
  my $esexip="192.168.115.99";
  my $esexport="2701";
  my $esex;
my $sensor;
  my $dummy;
  my $temp;
 
  $esex = Net::Telnet->new || die "kann Ethersex nicht finden";;
  $esex->open(Host  => $esexip,
            Port    => $esexport,
            Timeout => 2);
 
  $esex->print("brewset 0 23 2");
  ($sensor) = $esex->waitfor(Timeout => 2,
                             String  => "OK");
  $esex->print("brewset 1 26 3");
  ($sensor) = $esex->waitfor(Timeout => 2,
                             String  => "OK");


  $esex->print("brewsave");
  ($sensor) = $esex->waitfor(Timeout => 2,
                             String  => "OK");

  

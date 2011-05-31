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
 
  $esex->print("brewset 0 60 5");
  ($sensor) = $esex->waitfor(Timeout => 2,
                             String  => "OK");
  $esex->print("brewset 1 57 10");
  ($sensor) = $esex->waitfor(Timeout => 2,
                             String  => "OK");
  $esex->print("brewset 2 63 45 ");
  ($sensor) = $esex->waitfor(Timeout => 2,
                             String  => "OK");
  $esex->print("brewset 3 73 20 ");
  ($sensor) = $esex->waitfor(Timeout => 2,
                             String  => "OK");

  $esex->print("brewset 4 78 1 ");

  ($sensor) = $esex->waitfor(Timeout => 2,
                             String  => "OK");
  $esex->print("brewsave");
  ($sensor) = $esex->waitfor(Timeout => 2,
                             String  => "OK");

  

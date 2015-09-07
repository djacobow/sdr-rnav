#!/usr/bin/perl -w

use warnings qw(all);
use strict;
use Data::Dumper;

use djdsp;

radio_init();

sleep 1;

radio_enable_mixer(0);
#radio_enable_fft(1);
radio_set_frequency(88500000);

my $end = time + 60;

while (time < $end) {

 my $status = radio_get_status();
 print Dumper $status;

 my $fstat  = radio_get_fft();
 print Dumper $fstat;
 sleep 1;

}

radio_shutdown();


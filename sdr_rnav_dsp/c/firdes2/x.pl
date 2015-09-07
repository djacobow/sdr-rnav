#!/usr/bin/perl -w

use oct_remez;
use Data::Dumper;


my $num_taps  = 21;
my $num_bands = 2;
my $bands     = [ 0, 0.4, 0.5, 1];
my $des       = [ 1, 1, 0, 0 ];
my $weights   = [ 1, 28.013];
my $type      = 1;
my $density   = 32;

my $x = oct_remez::run_remez($num_taps, $num_bands, $bands, $des, $weights, $type, $density);

print Dumper $x;

1;


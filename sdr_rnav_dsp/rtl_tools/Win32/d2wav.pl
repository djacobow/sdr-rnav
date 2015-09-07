#!/usr/bin/perl -w
use strict;
use warnings qw(all);

use Audio::Wav;

my $aw = new Audio::Wav;
my $writer = $aw->write("foo.wav", {  bits_sample => 16, sample_rate => 256000, channels => 2 });

my $ifh = undef;
my $fn = 'foo.dat';

open($ifh,"<",$fn) or die "could not open $fn\n";

binmode($ifh);

my $data = '';
while (!eof($ifh)) {
 my $dread = read($ifh,$data,1024);
 for (my $i=0;$i<$dread;$i++) {
  my $byte = substr($data,$i,1);
  $writer->write_raw(pack('s',ord($byte)+0));
 }
};

$writer->finish();


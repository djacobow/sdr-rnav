#!/usr/bin/perl -w

# this is cribbed directly from the gnuradio filter order estimation
# the routines here help seed the 'remez' filter designor function with 
# the appropriate number of taps required.

package filt_ord_est;

use Exporter qw(import);
use Data::Dumper;

use POSIX;

our @EXPORT = qw(lporder remezord stopband_atten_to_dev passband_ripple_to_dev mylog10);

sub mylog10 {
 return log($_[0]) / log(10);
}

sub lporder {
 my $freq1 = shift;
 my $freq2 = shift;
 my $delta_p = shift;
 my $delta_s = shift;

 my $df  = abs($freq2 - $freq1);
 my $ddp = mylog10($delta_p);
 my $dds = mylog10($delta_s);
 my $a1 = 5.309e-3;
 my $a2 = 7.114e-2;
 my $a3 = -4.761e-1;
 my $a4 = -2.66e-3;
 my $a5 = -5.941e-1;
 my $a6 = -4.278e-1;
 my $b1 = 11.01217;
 my $b2 = 0.5124401;
 my $t1 = $a1 * $ddp * $ddp;
 my $t2 = $a2 * $ddp;
 my $t3 = $a4 * $ddp * $ddp;
 my $t4 = $a5 * $ddp;
 my $dinf=(($t1 + $t2 + $a3) * $dds) + ($t3 + $t4 + $a6);
 my $ff = $b1 + $b2 * ($ddp - $dds);
 my $n = $dinf / $df - $ff * $df + 1;
 return $n;

};

sub max3 {
 return ($_[0] > $_[1]) ? 
  (($_[0] > $_[2]) ? $_[0] : $_[2]) :
  (($_[1] > $_[2]) ? $_[1] : $_[2]);
};

sub max {
 my $a = shift;
 my $max = $a->[0];
 for (my $i=1; $i<@$a;$i++) {
  if ($a->[$i] > $max) { $max = $a->[$i]; };
 }
 return $max;
};

sub remezord {
 my $fcuts = shift;
 my $mags  = shift;
 my $devs  = shift;
 my $fsamp = shift || 2;

 for (my $i=0;$i < scalar @$fcuts; $i++) {
   $fcuts->[$i] /= $fsamp;
 }
 my $nf = scalar @$fcuts;
 my $nm = scalar @$mags;
 my $nd = scalar @$devs;

 my $nbands = $nm;

 if ($nm != $nd) {
  die "Length of mags and devs must be equal";
 }
 if ($nf != 2 * ($nbands - 1)) {
  die "Length of f must be 2 * len (mags) - 2";
 }

 for (my $i=0; $i< scalar @$mags; $i++) {
  if ($mags->[$i] != 0) {
   $devs->[$i] /= $mags->[$i];
  }
 }

 my $f1 = [];
 my $f2 = [];
 for (my $i=0; $i< ((scalar @$fcuts)/2); $i++) {
  $f1->[$i] = $fcuts->[2*$i]; 
  $f2->[$i] = $fcuts->[2*$i+1]; 
 };

 my $n = 0;
 my $min_delta = 2;

 for (my $i=0; $i< scalar @$f1; $i++) {
  if (($f2->[$i] - $f1->[$i]) < $min_delta) {
   $n = $i;
   $min_delta = $f2->[$i] - $f1->[$i];
  }
 }

 my $l;
 if ($nbands == 2) {
  $l = lporder ($f1->[$n], $f2->[$n], $devs->[0], $devs->[1]);
 }  else {
  $l = 0;
  for (my $i=1;$i< $nbands-1; $i++)  {
   my $l1 = lporder ($f1->[$i-1], $f2->[$i-1], $devs->[$i], $devs->[$i-1]);
   my $l2 = lporder ($f1->[$i],   $f2->[$i],   $devs->[$i], $devs->[$i+1]);
   $l = max3($l, $l1, $l2);
  }
 }

 $n = int (ceil ($l)) - 1;               # need order, not length for remez

 my $ff = [0, @$fcuts, 1];
 for (my $i=1; $i < (@$ff - 1); $i++) {
  $ff->[$i] *= 2;
 }

 my $aa = [];
 foreach my $a (@$mags) {
  push(@$aa, $a, $a);
 }

 my $max_dev = max($devs);
 my $wts = [];
 for (my $i=0;$i<@$devs;$i++) {
  $wts->[$i] = 1;
 }
 for (my $i=0;$i<@$wts;$i++) {
  $wts->[$i] = $max_dev / $devs->[$i];
 }
 return ($n, $ff, $aa, $wts)
}


sub stopband_atten_to_dev {
 my $atten_db = shift;
 return 10**(-$atten_db/20);
};

sub passband_ripple_to_dev {
 my $ripple_db = shift;
 return (10**($ripple_db/20)-1)/(10**($ripple_db/20)+1);
}


__END__



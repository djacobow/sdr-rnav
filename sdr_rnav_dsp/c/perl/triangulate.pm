# Author: David Jacobowitz
#         david.jacobowitz@gmail.com
#
# Date  : Fall 2013
#
# Copyright 2013, David Jacobowitz

# Routines to take two known fixed points and two bearings from 
#  those points to determine a third position.
#

# TODO: THIS ROUTINE IS IN REALITY BROKEN -- it treats lat and lon
#  as X and Y coordinates on a cartesian plane. It should be treating
#  them as angles on a sphere, or better yet, a "WGS84." 

package triangulate;

use strict;
use warnings qw(all);
use Math::Trig;
use Data::Dumper;
use Exporter qw(import);

our @EXPORT = qw/triangCartesian/;

my $TPI = 2 * 3.14159265358;
my $r2d = 180 / 3.14159265358;
my $q_det_thresh = 1e-6;

# This routine is not correct for this application, but may be 
# close enough for closely spaced fixes in North America:
# will treat lat/lon as X/Y coordinates, which of course, they are
# *NOT*.
#
# Will replace with a great-circle aware version of this routine
# when I get around to it!
#  -- dgj
sub triangCartesian {
 my $p1 = shift;
 my $p2 = shift;

 my $res = {
  solution => 0,
 };

 #print Dumper $p1;
 #print Dumper $p2;

 # convert from compass angles to math textbook angles
 my $p1th     = fixAngles(90 - $p1->{r});
 my $p2th     = fixAngles(90 - $p2->{r});
 #print "p1th $p1th p2th $p2th\n";

 # calculate the distance between the two known fixes
 my $lC       = dist2($p1,$p2);
 #print "lC $lC\n";

 # calculate the angle, relative to the X axis, of the two known
 # fixes. Adjust it so that it increases and is positive clockwise
 my $th12     = fixAngles($r2d * atan2($p2->{y}-$p1->{y},$p2->{x}-$p1->{x}));
 #print "th12 $th12\n";

 # calculate the angles of all the corners of the triangle 
 # formed by the two known fixes and the fix to be determined.
 # This requires summing partial angles between axes and fixes 
 my $th_a     = $p1th - $th12;
 my $th_b     = 180 - $p2th + $th12;
 my $th_c     = 180 - $th_a - $th_b;
 #print "th_a $th_a th_b $th_b th_c $th_c\n";

 # calculate the lengths of the other sides of the triangle
 my $div0 = abs($th_c / 180);
 $div0 -= int($div0);
 $div0 *= 180;
 if ($div0 < 2) {
  $res->{reason} = 'collinear_radials';
  return $res;
 }
 my $lB = (sin($th_a/$r2d) / sin($th_c/$r2d)) * $lC;
 my $lA = (sin($th_b/$r2d) / sin($th_c/$r2d)) * $lC;
 #print "lB $lB lA $lA\n";

 # calculate the relative positions of the unknown fix relative
 # to the first point
 my $dx_p1    = $lA * cos($p1th/$r2d);
 my $dy_p1    = $lA * sin($p1th/$r2d);
 #print "dx_p1 $dx_p1 dy_p1 $dy_p1\n";

 # calculate the absolute position of the previous unknown, now
 # known fix
 my $soln_x = $p1->{x} + $dx_p1;
 my $soln_y = $p1->{y} + $dy_p1;
 $res->{x} = $soln_x;
 $res->{y} = $soln_y;

 # detect if this is a mirror solution to radials that diverge
 #  note: this is probably not the best or even a correct way
 #  to do this -- need to revisit!
 my $slope_p2 = { y => sin($p2th/$r2d), x => cos($p2th/$r2d), };
 my $p2_q = {
  x => ($slope_p2->{x} >  $q_det_thresh) ?  1 :
       ($slope_p2->{x} < -$q_det_thresh) ? -1 : 0,
  y => ($slope_p2->{y} >  $q_det_thresh) ?  1 :
       ($slope_p2->{y} < -$q_det_thresh) ? -1 : 0,
 };
 #print Dumper $p2_q;

 my $dx_p2 = $soln_x - $p2->{x};
 my $dy_p2 = $soln_y - $p2->{y};
 #print "dx_p2 $dx_p2 dy_p2 $dy_p2\n";

 my $p2_soln_q = {};
 $p2_soln_q->{x} = ($dx_p2 > $q_det_thresh) ? 1 : ($dx_p2 < -$q_det_thresh) ? -1 : 0;
 $p2_soln_q->{y} = ($dy_p2 > $q_det_thresh) ? 1 : ($dy_p2 < -$q_det_thresh) ? -1 : 0;
 #print Dumper $p2_soln_q;

 my $diverge = ($p2_soln_q->{x} != $p2_q->{x}) || ($p2_soln_q->{y} != $p2_q->{y});
 if ($diverge) {
  $res->{reason} = 'divergent_radials';
  return $res;
 }


 $res->{solution} = 1; 
 return $res;
};

sub dist2 {
 my $p1 = shift;
 my $p2 = shift;
 return sqrt(($p2->{x}-$p1->{x})**2+($p2->{y}-$p1->{y})**2);
};

sub fixAngles {
 my $a = shift;
 while ($a < 0) { $a += 360; };
 while ($a > 360) { $a -= 360; };
 return $a;
};

1;


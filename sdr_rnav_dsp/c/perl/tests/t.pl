#/usr/bin/perl -w

use triangulate;

use strict;
use warnings qw(all);
use Data::Dumper;

print "should DIVERGE\n";
print Dumper
triangCartesian(
        { x => 0,    y => 0, r => 45, },
	{ x => 10,   y => 11, r => 0, }, 
	);


print "should OK\n";
print Dumper
triangCartesian(
        { x => 0,    y => 0, r => 45, },
	{ x => 10,   y => 9, r => 0, }, 
	);

print "should DIVERGE\n";
print Dumper
triangCartesian(
        { x => 0,    y => 0, r => 315, },
	{ x => -10,  y => 11, r => 0, }, 
	);

print "should OK\n";
print Dumper
triangCartesian(
        { x => 0,    y => 0, r => 315, },
	{ x => -10,  y => 9, r => 0, }, 
	);
__END__


print Dumper
triangCartesian({ x => 10 , y => 0, r => 355, }, 
	        { x => 0,   y => 0, r => 5, },);

print Dumper
triangCartesian({ x => 0 , y => 0,  r => 60, }, 
	        { x => 10, y => -4, r => 330, },);

print Dumper
triangCartesian({ x => 0, y => 0, r => 30, },
	        { x => 10, y => -4, r => 360, },);

print Dumper
triangCartesian({ x => 0, y => 0, r => 350, },
	        { x => -10, y => -4, r => 10, },);

print Dumper
triangCartesian({ x => 0, y => 0, r => 45, },
	        { x => 10, y => 4, r => 360, },);

print Dumper
triangCartesian({ x => 0, y => 0, r => 350, },
	        { x => -10, y => 4, r => 360, },);

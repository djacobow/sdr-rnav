#!/usr/bin/perl -w

# fix dependencies generated by gcc so they handle 
# directories properly.

use warnings qw(all);
use strict;

my $objfn = $ARGV[0];
my $depfn = $ARGV[1];

sub slurp {
 my $fn = shift;
 local $/ = undef;
 my $fh;
 open($fh, '<', $fn);
 my $str = <$fh>;
 close $fh;
 return $str;
}

sub splat {
 my $fn = shift;
 my $str = shift;
 my $fh;
 open($fh, '>', $fn);
 print $fh $str;
 close $fh;
}

my $str = slurp($depfn);
unlink $depfn;
$str =~ s/^.*://;
if ($^O eq 'MSWin32') {
 $objfn =~ s/\\/\//g;
}
$str = $objfn . ":" . $str;
splat($depfn,$str);

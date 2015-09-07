#!/usr/bin/perl -w

use strict;
use warnings qw(all);

use Data::Dumper;


my $infile = defined($ARGV[0]) ? $ARGV[0] : "dp_base.h";

my $restricted = {
 linux_only => [ 'linaudio' ],
 never => [ 'null', 'invalid', 'ctors' ],
};

my $classes  = getClasses($infile);
my $filtered = filterClasses(filters => $restricted, class_list => $classes);

makeCtors(out_base => "_dp_ctors", class_list => $filtered);



sub filterClasses {
 my %args = @_;
 my $filters = $args{filters};
 my $classes = $args{class_list};

 my $o = {};
 foreach my $class (@$classes) {
  $o->{$class} = 1;
 }

 foreach my $filter (keys %$filters) {
  foreach my $item (@{$filters->{$filter}}) {
   if (($filter eq 'linux_only') && ($^O ne 'Linux')) {
    delete $o->{$item};
   } elsif (($filter eq 'windows_only') && ($^O ne 'Win32')) {
    delete $o->{$item};
   } elsif (($filter eq 'never')) {
    delete $o->{$item};
   }
  }
 };
 my @filtered = keys %$o;
 return \@filtered;
};

sub makeCtors {
 my %args = @_;

 my $out_base = $args{out_base};
 my $classes  = $args{class_list};

 my $outfn_h  = $out_base . ".h";
 my $outfn_c  = $out_base . ".c";

 my ($hfh, $cfh);
 open($hfh, '>', $outfn_h) or die "-error could not open $outfn_h\n";
 open($cfh, '>', $outfn_c) or die "-error could not open $outfn_c\n";

 print $hfh "#ifndef _DP_CTORS_H\n#define _DP_CTORS_H\n\n";
 print $hfh "#include \"dp_base.h\"\n";

 # print $cfh "#include \"dp_ctors.h\"\n\n";

 foreach my $cname (@$classes) {
  print "cname $cname\n";
  print $cfh "#include \"dp_" . $cname . ".h\"\n";
 }


 foreach my $cname (@$classes) {
  my $ct_name = "dp_" . $cname . "_create_from";

  my $h_string = << "_end_of_h_here";
dp_base_t *dp_create_$cname();
_end_of_h_here

  my $c_string = << "_end_of_c_here";
dp_base_t *dp_create_$cname() {
 dp_base_t *b;
 b = dp_base_create();
 $ct_name(b);
 return b;
}

_end_of_c_here

  print $cfh $c_string;
  print $hfh $h_string;
 };

 print $hfh "\n\n#endif\n";

 close $hfh;
 close $cfh;
};


sub getClasses{
 my $infn = shift;
 my $results = [];
 my $fh = undef;
 open($fh,'<',$infn) or die "-error could not open $infn for read\n";
 my $in_typedef = 0;
 while (<$fh>) {
  my $l = $_; chomp $l;
  if ($l =~ /typedef enum dp_subt_t/) {
   $in_typedef = 1;
  } elsif ($l =~ /\} dp_subt_t;/) {
   $in_typedef = 0;
  } elsif ($in_typedef) {
   my $t = $l;
   $t =~ s/\s//g;
   $t =~ s/,//g;
   if ($t !~ /invalid|null/) {
    $t =~ s/dp_subt_//;
    push(@$results,$t);
   }
  }
 };
 return $results;
};


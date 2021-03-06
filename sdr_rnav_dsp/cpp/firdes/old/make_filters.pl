#!/usr/bin/perl -w

# Routine to scan for strings in c / c++ files and build filters
# based on parameters found in that string, then put the resulting
# filter kernels in files that can be included in the compilation
#
# calls firdes.exe which is a wrapper around the "remez" routines
# found in matlab and other places.
#
# Author: djacobow
# Date  : May 2013


use strict;
use warnings qw(all);

use YAML qw(freeze thaw);
use Data::Dumper;
use Storable qw(dclone);


my $fir_program = 'firdes.exe';

@ARGV = qw(filter_kernels_auto test.cpp test.h);

my $output_base_name = shift @ARGV;
my @files_to_scan    = @ARGV;

my $fdata   = readFiles(@files_to_scan);
my $reqs    = getFilterReqs($fdata);
my $filters = makeFilters($reqs);

writeResults($output_base_name, $filters, 'int16_t',
	     \@files_to_scan);





sub writeResults {
 my $ofbase  = shift;
 my $filters = shift;
 my $type    = shift;
 my $files   = shift;

 if (!defined($type)) { $type = 'int16_t'; }

 my $h_name = $ofbase . '.h';
 my $c_name = $ofbase . '.c';

 my ($hfh, $cfh) = (undef, undef);

 open($hfh,'>',$h_name) or die "-error- could not open output file $h_name\n";
 open($cfh,'>',$c_name) or die "-error- could not open output file $h_name\n";

 my $scanned_files     = join("\n * ",@$files);
 my $scanned_variables = join("\n * ",sort keys %$reqs);
 my $date_str = scalar localtime time;
 my $header = << "__end_of_header";



/*
 * Filter Coefficients
 *
 * This file was automatically generated by the script:
 *
 * $0 
 *
 * by scanning the following files for specially named variables that 
 * cleverly (?) include the parameters of filters that those variables 
 * are supposed to contains.
 *
 * The follow files were scanned:
 *
 * $scanned_files
 *
 * In that scan, the follow variables were found:
 *
 * $scanned_variables 
 *
 * Note that it is possible that some of those listed above had errors 
 * (which should have been reported during the make process, or the 
 * firdes subroutine just croaked, in which case those filters will 
 * not be present below and very likely the program that was expected
 * them to be there will not compile. :-(
 *
 *
 * Ran on $date_str
 */


__end_of_header

 print $hfh $header;
 print $cfh $header;

 foreach my $fname (keys %$filters) {
  
  my $filter = $filters->{$fname};

  my $len_name  = $fname . '_len';
  my $taps_name = $fname . '_coeffs';

  my $reqs = dclone($filter);
  delete $reqs->{taps};
  my $rstring = freeze($reqs);
  print $hfh "/*\n";
  print $hfh $rstring;
  print $hfh "*/\n";
  print $cfh "/*\n";
  print $cfh $rstring;
  print $cfh "*/\n\n";

  print $hfh 'extern const uint32_t ' . $len_name . ";\n";
  print $cfh 'const uint32_t ' . $len_name . ' = ' . $filter->{number_taps} . ";\n";

  print $hfh 'extern const ' . $type . ' ' . $taps_name . '[' . $filter->{number_taps} . "];\n";
  print $cfh 'const ' . $type . ' ' . $taps_name . '[' . $filter->{number_taps} . '] = {' . "\n";

  my @taps = map { ($type eq 'int16_t') ? int(32768*$_) :
                   ($type eq 'int32_t') ? int(2147483647*$_) :
		                          $_ } @{$filter->{taps}};
  print $cfh join(', ', @taps);
  print $cfh "\n};\n\n\n\n\n\n\n\n";

 };
 close $hfh;
 close $cfh;

};



sub makeFilters {
 my $reqs = shift;

 my $results = {};
 foreach my $rname (keys %$reqs) {
  my $req = $reqs->{$rname};
  if (scalar @{$req->{errors}}) {
   print "-error- filter $req->{name} had errors; not running\n";
  } else {
   if ($req->{type} eq 'fir') {
    my $command = join(' ',
     $fir_program,
     'taps'     , $req->{num_taps},
     'type'     , $req->{subtype},
     'num_bands', (scalar @{$req->{bands}}) / 2,
     'bands'    , @{$req->{bands}},
     'gains'    , @{$req->{gains}});
 
    if (defined($req->{weights})) {
     $command .= join(' ','weights'  , @{$req->{weights}});
    }

    print "-info- running command:\n";
    print " " . $command . "\n";

    my $returned = qx/$command/;
    if (length($returned) && ($returned=~ /^---/)) {
     $results->{$rname} = thaw($returned); 
     $results->{$rname}{name} = $rname;
    }
   } else {
    print "-warn- filter type $req->{type} not yet impleented\n";
   } 
  }
 };
 return $results;
};








sub slurpFile {
 my $ifn = shift;
 my $fh  = undef;
 my $os  = '';
 open($fh,'<',$ifn);
 if (defined($fh)) {
  while (<$fh>) {
   $os .= $_;
  }
 } else {
  print "-warn- could not open file $ifn\n";
 }
 return $os;
};


sub readFiles {
 my @files = @_;

 my $is = '';
 foreach my $file (@files) {
  if (-e $file) {
   $is .= slurpFile($file); 
  };
 };
 return $is;
};


sub dB_to_num {
 my $in = shift;
 if ($in =~ /^(n?)(\d+)$/) {
  my $db  = $2;
  if ($1 eq 'n') { $db = -$db; };
  return 10**($db/10);
 }
 return undef;
};

sub engToNum {
 my $in = shift;
 my $num_part = undef;
 my $mod_part = undef;

 if ($in =~ /(\d+\.?\d*)(\w?)/) {
  $num_part = $1;
  $mod_part = $2;
 }
 my $power = 0;
 if (defined($mod_part) && length($mod_part)) {
  if ($mod_part eq 'f') { $power = -15; };
  if ($mod_part eq 'p') { $power = -12; };
  if ($mod_part eq 'n') { $power = -9; };
  if ($mod_part eq 'u') { $power = -6; };
  if ($mod_part eq 'm') { $power = -3; };
  if ($mod_part eq 'k') { $power =  3; };
  if ($mod_part eq 'M') { $power =  6; };
  if ($mod_part eq 'G') { $power =  9; };
  if ($mod_part eq 'T') { $power = 12; };
  if ($mod_part eq 'P') { $power = 15; };
  if ($mod_part eq 'E') { $power = 18; };
 }
 return $num_part * 10**$power;
};



sub getFilterReqs {
 my $is = shift;
 my @matches = $is =~ /_filt_type_.*_coeffs/g;
 my $reqs = {};
 foreach my $match (@matches) {
  $match =~ s/_coeffs$//;
  my $req = { name => $match, errors => []};
 
  if ($match =~ /_filt_type_FIR/) {
   $req->{type} = 'fir';
   if ($match =~ /sr_(\d+\w?)/) {
    $req->{sample_rate} = engToNum($1);
   } else {
    push(@{$req->{errors}},'no_sample_rate');
   }
   if ($match =~ /bands_((\d+\w?_)+)/) {
    my @bands = map { engToNum($_) / $req->{sample_rate} } split(/_+/,$1);
    foreach my $be (@bands) {
     if ($be > 0.5) {
       push(@{$req->{errors}},'bandedge_above_nyquist');
     }
     $req->{bands} = \@bands;
    };
   }
   if ($match =~ /taps_(\d+)/) {
    $req->{num_taps} = $1;
   }
   if ($match =~ /weights_((\d+_)+)/) {
    my @weights = split(/_+/,$1);
    $req->{weights} = \@weights;
   }
   if ($match =~ /gains_((n?\d+_)+)/) {
    my @gains = split(/_+/,$1);
    @gains = map { dB_to_num($_); } @gains;
    $req->{gains} = \@gains;
   }
   my $subtype = 1;
   if ($match =~ /type_(\w+)/) {
    if ($1 =~ /^band/) {
     $subtype = 1;
    } elsif ($1 =~ /^diff/) {
     $subtype = 2;
    } elsif ($1 =~ /^hilb/) {
     $subtype = 3;
    }
   }
   $req->{subtype} = $subtype;
   $reqs->{$match} = $req;
  };
 }
 return $reqs;
};


__END__



my $command = "firdes.exe taps 100 num_bands 2 bands 0 0.02 0.1 1 gains 1 0.00001";

my $res = `$command`;

# print $res; exit;
my $ds  = thaw($res);

print Dumper $ds;



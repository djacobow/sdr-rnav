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

use FindBin;
use lib "$FindBin::Bin";

use strict;
use warnings qw(all);

use YAML qw(freeze thaw);
use Data::Dumper;
use Storable qw(dclone);
use filt_ord_est;

my $fir_program = $FindBin::Bin . (($^O eq 'MSWin32') ? '\\' : '/') . 'firdes.py';

my $output_base_name = shift @ARGV;
my @files_to_scan    = @ARGV;

my $fdata   = readFiles(@files_to_scan);
my $reqs    = getFilterReqs($fdata);
my $results = makeFilters($reqs);

writeResults($output_base_name, $reqs, $results,
	     \@files_to_scan);





sub writeResults {
 my $ofbase  = shift;
 my $reqs    = shift;
 my $results = shift;
 my $files   = shift;

 # if (!defined($type)) { $type = 'int16_t'; }

 my $h_name = $ofbase . '.h';
 my $c_name = $ofbase . '.c';

 if (-e $h_name) { unlink $h_name; };
 if (-e $c_name) { unlink $c_name; };

 my $scanned_files     = join("\n * ",@$files);
 my @asked_filters = sort keys %$reqs;
 my @filter_list;
 my $filters_ok = 1;
 foreach my $asked_filter (@asked_filters) {
  if (defined($results->{$asked_filter}{taps})) {
   push(@filter_list,$asked_filter);
  } else {
   push(@filter_list,$asked_filter . " (missing / fail)");
   $filters_ok = 0;
  }
 }

 if (!$filters_ok) { 
  print "-Err- Some filters failed. Skipping writing results.\n";
  print join("\n",@filter_list);
  return;
 };

 my ($hfh, $cfh) = (undef, undef);
 open($hfh,'>',$h_name) or die "-error- could not open output file $h_name\n";
 open($cfh,'>',$c_name) or die "-error- could not open output file $h_name\n";

 my $scanned_variables = join("\n * ",sort @filter_list);
 my $date_str = scalar localtime time;
 my $header = << "__end_of_header";

#include <inttypes.h>


/*
 * Filter Coefficients
 *
 * This file was automatically generated by the script:
 *
 * $0 
 *
 * by scanning the following files for specially formatted comments
 * that cleverly (?) include the parameters of filters that will be 
 * referenced in the code in that file.  Included in the make process,
 * this allows changes in filter parameters in C files to immediately
 * be realized on the next compile, without having to copy/paste from
 * another tool.
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

 my $h_short_name = $h_name;
 $h_short_name =~ s/^.*\///;
 print $cfh "\n#include \"$h_short_name\"\n\n\n";

 foreach my $fname (keys %$reqs) {
  
  my $req    = dclone($reqs->{$fname});

  my $type   = $req->{dtype};
  my $result = $results->{$fname};
  if (!defined($result->{taps})) { next; }

  my $len_name  = $fname . '_len';
  my $taps_name = $fname . '_coeffs';

  my $rstring = freeze($req);
  print $hfh "/*\n";
  print $hfh $rstring;
  print $hfh "*/\n";
  print $cfh "/*\n";
  print $cfh $rstring;
  print $cfh "*/\n\n";

  my $num_taps = scalar @{$result->{taps}};

  print $hfh 'extern uint32_t ' . $len_name . ";\n";
  print $cfh 'uint32_t ' . $len_name . ' = ' . $num_taps . ";\n";

  print $hfh 'extern ' . $type . ' ' . $taps_name . '[' . $num_taps . "];\n";
  print $cfh '' . $type . ' ' . $taps_name . '[' . $num_taps . '] = {' . "\n";

  my @taps = map { ($type eq 'int16_t') ? int(32768*$_) :
                   ($type eq 'int32_t') ? int(2147483647*$_) :
		                          $_ } @{$result->{taps}};
  print $cfh join(', ', @taps);
  print $cfh "\n};\n\n\n\n\n\n\n\n";

 };
 close $hfh;
 close $cfh;

};


sub makeOctaveString {
 my $req = shift;

 my $os = '';

 $os .= "bands   = [" . join(',',@{$req->{remez_bands}}) . "];\n";
 $os .= "gains   = [" . join(',',@{$req->{remez_mags}}) . "];\n";
 if (defined($req->{remez_weights})) {
  $os .= "weights = [" . join(',',@{$req->{remez_weights}}) . "];\n";
 }
 $os .= "num_taps = " . $req->{remez_num_taps} . ";\n";
 $os .= "type = '" . $req->{remez_type} . "';\n";

 $os .= "b = remez(num_taps, bands, gains, weights, 'bandpass')\n";
 #$os .= "b *= 32767;\n";
 #$os .= "b = round(b)\n";

 return $os;
};

sub processOctaveResults {
 my $is = shift;
 my $oa = [];
 my @lines = split(/\n/,$is);
 my $good_data = 0;
 while (@lines) {
  my $line = shift @lines;
  if ($line =~ /^b =/) {
   $good_data = 1;
   shift @lines;
  } elsif ($good_data) {
   chomp $line;
   if ($line =~ /(-?\d+.*)/) {
    push(@$oa, $1);
   }
  }
 }
 return $oa;
};


sub run_prog_w_stdin { 
 my $p = shift;
 my $i = shift;
 print "make_filters.pl calling $p < $i\n";
 return `echo \"$i\" | \"$p\"`;
};


sub makeFilters {
 my $reqs = shift;

 my $results = {};
 foreach my $rname (keys %$reqs) {
  my $req = $reqs->{$rname};

  my @bands_copy = map { 1*$_; } @{$req->{bands}}; shift @bands_copy; pop @bands_copy;

  my ($n, $f, $aa, $wts) = remezord(\@bands_copy,$req->{mags},$req->{devs},
$req->{sample_rate});

  $req->{remez_bands}    = $f;
  $req->{remez_num_taps} = $n + 2;
  $req->{remez_mags}     = $aa;
  $req->{remez_weights}  = $wts;
  $req->{remez_type}     = $req->{subtype};
  # allow user to override remezord's guess
  #if (defined($req->{num_taps}) && ($req->{num_taps} > $req->{remez_num_taps})) {
  if (defined($req->{num_taps})) {
   $req->{remez_num_taps} = $req->{num_taps};
  }

  if (scalar @{$req->{errors}}) {
   print "-error- filter $req->{name} had errors; not running\n";
  } else {
   if ($req->{type} eq 'fir') {
    if ($^O ne 'MSWin32') {
     my $ostring = makeOctaveString($req);
     my $octavprog = 'octave';
 
     my $returned = run_prog_w_stdin($octavprog,$ostring);
     my $ret_array = [];
     if (length($returned)) {
      # print Dumper $returned;
      $ret_array = processOctaveResults($returned); 
     }
     if (@$ret_array) {
      $results->{$rname} = {
       taps => $ret_array,
       num_taps => scalar @$ret_array,
       name => $rname
      };
     } else {
      push(@{$req->{errors}}, 'remez failed');
     }
    } else {
     my $command = join(' ',
      $fir_program,
      'num_taps' , $req->{remez_num_taps},
      'type'     , $req->{remez_type},
      'num_bands', (scalar @{$req->{bands}}) / 2,
      'bands'    , @{$req->{remez_bands}},
      'gains'    , @{$req->{remez_mags}});
  
     if (defined($req->{remez_weights})) {
      $command .= join(' ',' weights'  , @{$req->{remez_weights}});
     }
 
     print "-info- running command (filter $rname):\n";
     print " " . $command . "\n";
 
     my $returned = qx/$command/;
 
     if (length($returned) && ($returned=~ /^---/)) {
      $results->{$rname} = thaw($returned); 
      $results->{$rname}{name} = $rname;
     } else {
      push(@{$req->{errors}}, 'remez failed');
     }
    }
   } else {
    print "-warn- filter type $req->{type} not yet impleented\n";
   } 
  }
 };
 return $results;
};





sub readFiles {
 my @files = @_;
 my $rdata = [];
 my $is = '';
 foreach my $file (@files) {
  if (-e $file) {
   my $fh = undef;
   open($fh,'<',$file);
   if (defined($fh)) {
    my $reading_yaml = 0;
    my $ystring = '';
    while (<$fh>) {
     my $il = $_; chomp $il;
     if ($il =~ /__firdes__start__yaml__/) {
      $reading_yaml = 1;
     } elsif ($il =~ /__firdes__end__yaml__/) {
      $reading_yaml = 0;
      my $ydata = thaw($ystring);
      foreach my $ydatum (@$ydata) {
       push(@$rdata, $ydatum);
      }
      $ystring = '';
      # thaw
     } elsif ($reading_yaml) {
      $ystring .= $_ . "\n";
     }
    }
    close $fh;
   }
  };
 };
 return $rdata;
};


sub engToNum {
 my $in  = shift;
 my $res = undef; 

 if      ($in =~ /(-?\d+\.?\d*)dB/) {
  my $nval = $1;
  $res = 10**($nval/10);
 } elsif ($in =~ /(-?\d+\.?\d*) ?(\w?)/) {
  # convert from engineering units
  my $num_part = $1;
  my $mod_part = $2;
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
  $res = $num_part * 10**$power;
 } else {
  # something has gone wrong! 
  print "-error- in is $in\n";
  return undef;
 }
 return $res;
};


sub useIfDefined {
 my $a = shift;
 my $b = shift;
 my $c = shift;
 return (defined($a) ? $a : defined($b) ? $b : $c);
};

sub getFilterReqs {
 my $ydata = shift;

 my $reqs  = shift;
 my $ct = 0;
 foreach my $ydatum (@$ydata) {
  my $name = useIfDefined($ydatum->{name}, "auto_filter_" . $ct);
  my $req = {
   name        => $name,
   sample_rate => engToNum(useIfDefined($ydatum->{sample_rate},44100)),
   type        => useIfDefined($ydatum->{type}, 'fir'),
   subtype     => 1,
   #num_taps    => engToNum(useIfDefined($ydatum->{taps}, 20)),
   dtype       => useIfDefined($ydatum->{dtype}, "int16_t"),
   errors      => [],
  };

  if (!defined($ydatum->{sample_rate})) {
   push(@{$req->{errors}},'no_sample_rate');
  }

  my $subtype = useIfDefined($ydatum->{subtype},'bandpass');
  if    ($subtype eq 'differentiator') { $subtype = 2; }
  elsif ($subtype eq 'hilbert')        { $subtype = 3; }
  else                                 { $subtype = 1; }
  $req->{subtype} = $subtype;
  
  if (defined($ydatum->{bands})) {
   my @bands;
   foreach my $band (@{$ydatum->{bands}}) {
    push(@bands, (engToNum($band->{start}), engToNum($band->{end})));
   }
   # @bands = map { $_ / ($req->{sample_rate}/2) } @bands;
   $req->{bands} = \@bands;
   $req->{num_bands} = scalar @{$ydatum->{bands}};
  } else {
   push (@{$req->{errors}}, 'no_bands_provided');
  };
  if (defined($ydatum->{num_taps})) {
   $req->{num_taps} = $ydatum->{num_taps};
  }
  if (defined($ydatum->{mags})) {
   foreach my $gain (@{$ydatum->{mags}}) {
	   # push(@{$req->{mags}}, (engToNum($gain->{start}), engToNum($gain->{end})));
    push(@{$req->{mags}}, (engToNum($gain)));
   }
  } else {
   push (@{$req->{errors}}, 'no_mags_provided');
  };
  if (defined($ydatum->{ripple})) {
   foreach my $ripple (@{$ydatum->{ripple}}) {
    my $ripv  = engToNum($ripple);
    my $ripdb = 10 * mylog10($ripv);
    my $dev  = 0;
    if ($ripv > 1) {
     $dev = passband_ripple_to_dev($ripdb);
     # print "ripdb $ripdb, dev $dev\n";
    } else {
     $dev = stopband_atten_to_dev(-$ripdb);
     # print "ripdb $ripdb, dev $dev\n";
    }
    push(@{$req->{devs}}, $dev);
   }
  };

  $reqs->{$name}   = $req;
 };
 return $reqs;
};



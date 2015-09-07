#!/usr/bin/perl -w

# loads a list of filters to be designed from a YAML file,
# then generates the filters using a remez routine that is 
# itself part of gnuradio (or octave, if installed), then
# returns the generated filters.
#
# Will keep a cached version around to speed things up if the 
# YAML hasn't changed.
#
# Author: djacobow
# Date  : May 2013
package firdes;

use FindBin;
use lib "$FindBin::Bin";

use strict;
use warnings qw(all);

use Exporter qw(import);
our @EXPORT = qw(firdes);

use YAML     qw(freeze thaw LoadFile DumpFile);
use Data::Dumper;
use Storable qw(dclone);
use filt_ord_est;
use oct_remez;

my $fcachename  = '_firdes_cache.yaml';

sub firdes {
 my %args    = @_;
 my $files   = $args{files};
 my $embedded = $args{embedded} || 0;
 my $fdata   = [];
 if ($embedded) {
  $fdata   = readEmbeddedYAML(@$files);
 } else  {
  @$fdata = map { map { @$_; } LoadFile($_); } @$files; 
 }
 my $reqs    = getFilterReqs($fdata);
 my $cdata   = readCache($fcachename);
 my $files_age =  (sort { $b <=> $a } map { (stat $_)[9]; } @$files)[0];
 my $cache_age = (stat $fcachename)[9];
 if (!defined($cache_age)) {
  $cache_age = time() + 999999;
 }
 my $results = makeFilters(
	      reqs => $reqs, 
	      cache_data => $cdata,
              cache_age => $cache_age,
              files_age => $files_age,
               );
 writeCache($fcachename,$results);
 return $results;
};


sub writeCache {
 my $fn = shift;
 my $d  = shift;
 DumpFile($fn,$d);
};

sub readCache {
 my $fn = shift;
 my $cdata = {};
 if (-e $fn) {
  $cdata = LoadFile($fn);
 }
 return $cdata;
};

sub makeFilters {
 my %args = @_;
 my $reqs  = $args{reqs};
 my $cdata = $args{cache_data};
 my $c_age = $args{cache_age};
 my $f_age = $args{files_age};

 my $use_cache_if_available = ($f_age < $c_age);

 my $results = {};
 foreach my $rname (keys %$reqs) {
  if (defined($cdata->{$rname}) && $use_cache_if_available) {
   $results->{$rname} = $cdata->{$rname};
  } else {
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

     my @r_args = (
      $req->{remez_num_taps},
      scalar @{$req->{remez_bands}} / 2,
      $req->{remez_bands},
      $req->{remez_mags},
      $req->{remez_weights},
      1,
      32);
     print "rname $rname\n";
     print Dumper \@r_args;
     my $rv = oct_remez::run_remez(@r_args);
     $results->{$rname} = {
      taps => $rv,
      num_taps => scalar @$rv,
      name => $rname,
     };
     if (!defined($rv) || ! scalar @$rv) {
      die ("-pe- FIR filter designer failed on required filter. Can't proceed.\n");
     }
    } else {
     print "-warn- filter type $req->{type} not yet implemented\n";
    } 
   }
  }
 };
 return $results;
};





sub readEmbeddedYAML {
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

1;


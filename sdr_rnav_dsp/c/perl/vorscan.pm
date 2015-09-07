# Author: David Jacobowitz
#         david.jacobowitz@gmail.com
#
# Date  : Fall 2013
#
# Copyright 2013, David Jacobowitz

# an object that contains routines to
#  1. scan the entire VOR spectrum for signals of a certain
#     strength
#  2. see which of those signals might be a VOR
#  3. for each of those signals, sit on the signal long enough 
#     to decode its ID
#  4. for a list of ID'd signals, read radials and try to
#     calculate positions.

package vorscan;

use strict;
use warnings qw(all);

use Exporter qw(import);
our @EXPORT = qw/foo bar baz/;

use Data::Dumper;
use rcontrol;
use YAML qw(LoadFile);
use Time::HiRes qw(usleep);

sub new {
 my $c = shift;
 my $cfg = shift;

 my $s = {
  scfg => {
   vordb_file => '..\\src\\vor_db\\vordb.yaml',
   scan => {
    vor_f_range => [ 108000000, 117950000 ],
    #vor_f_range => [ 88000000, 105000000],
    min_dwell => 2 * 1e6,
    max_dwell => 3 * 1e6,
    max_per_hop => 5,
    peaks_thresh => 20,
   },
   qualify => {
    max_candidates => 2,
   },
   decode => {
    calibrate_radial => 195,
    sr => 250000,
    id_pre_dwell => 2 * 1e6,
    id_dwell => 30 * 1e6,
    min_dwell => 2 * 1e6,
    pos_dwell => 2 * 1e6,
   },
  },
  cfg => $cfg,
 };

 if ($^O eq 'linux') {
  $s->{scfg}{vordb_file} =~ s/\\/\//g;
 }
 $s->{vordata} = LoadFile($s->{scfg}{vordb_file});
 bless($s);
 return $s;
};



sub findSignals {
 my $s = shift;
 my $cfg = $s->{cfg};

 print "==== Starting signal scan ====\n";

 my $interesting_sigs = {};

 fft_mode_enable($cfg,1);
 
 my $f_start = int($s->{scfg}{scan}{vor_f_range}[0] + ($cfg->{srates}{fft} / 2));
 my $f_end   = int($s->{scfg}{scan}{vor_f_range}[1] - ($cfg->{srates}{fft} / 2));

 while ($f_start < $f_end) {
  change_freq($cfg,$f_start);
  usleep($s->{scfg}{scan}{min_dwell});
  my $start_time = time();
  my $done = 0;
  while (!$done) {
   my $r = try_pop_all($cfg->{fstat_queue});
   if (defined($r) && ($r->{have_fft})) {
    #print Dumper $r;
    my @candidates = sort { $b->{dB} <=> $a->{dB} } @{$r->{points}};
    my $ccount = scalar @candidates;
    $ccount = ($ccount > $s->{scfg}{scan}{max_per_hop}) ? $s->{scfg}{scan}{max_per_hop} : $ccount;
    for (my $i=0;$i<$ccount;$i++) {
     $interesting_sigs->{ $candidates[$i]->{f} } = $candidates[$i];
    }
    $done = 1;
   } elsif (time() > ($s->{scfg}{scan}{max_dwell} + $start_time)) {
    $done = 1;
   }
  }
  $f_start += $s->{cfg}{srates}{fft};
 };

 fft_mode_enable($cfg,0);

 foreach my $f (keys %$interesting_sigs) {
  printf("signal: f=%f scan_snr=%f\n",$f,$interesting_sigs->{$f}{dB});
 }

 print "==== signal scan complete ====\n";
 $s->{last}{sig_scan} = $interesting_sigs;
};


sub findVORs {
 my $s = shift;
 my $siglist = undef;
 if (defined($s->{last}{sig_scan})) {
  $siglist = $s->{last}{sig_scan}; 
 } else {
  print "-pe- findVORs called before signalScan\n";
  return undef;
 }

 my $vorlist = {};

 print "=== Starting VOR scan ===\n";

 my @vors_in_order = sort { $siglist->{$b}{dB} <=> $siglist->{$a}{dB} } keys %$siglist;
 print Dumper \@vors_in_order; 

 my $true_fs = {};
 foreach my $f (@vors_in_order) {
  # frequency spacing for VORs is 50kHz
  my $true_f = 50000 * int($f / 50000 + 0.5);
  if (defined($true_fs->{$true_f})) {
   push(@{$true_fs->{$true_f}},$f);
  } else {
   $true_fs->{$true_f} = [ $f ];
  }
 };

 foreach my $true_f (keys %$true_fs) {
  my @fs = @{$true_fs->{$true_f}};
  my $max_fs = scalar @fs;
  if ($max_fs > $s->{scfg}{qualify}{max_candidates}) { 
   $max_fs = $s->{scfg}{qualify}{max_candidates};
  }
  for (my $i=0;$i<$max_fs;$i++) {
   my $f = $fs[$i];
   change_freq($s->{cfg},$f);
   usleep ($s->{scfg}{decode}{min_dwell});
   my $done = 0;
   while (!$done) {
    my $r = try_pop_all($s->{cfg}{rstat_queue});
    if (defined($r && $r->{have_status})) {
     $done = 1;
     if ($r->{have_carrier}) {
      if (!defined($vorlist->{$true_f})) {
       $vorlist->{$true_f} = [];
      }
      push(@{$vorlist->{$true_f}}, 
       {
        f => $f,
        true_f => $true_f,
        scan_dB => $siglist->{$f}{dB},
        snr_dB => $r->{strength_ratio_lpf},
       });
      printf("looks_like_vor f:%f, true_f:%f, search dB:%f, snr:%f\n",$f,$true_f,$siglist->{$f}{dB},$r->{strength_ratio_lpf});
     }
    } else {
     print "-pd- no status\n";
     usleep(10000);
    }	   
   } 
  }
 };

 foreach my $tf (keys %$vorlist) {
  printf("vor: true_f:%f\n",$tf);
  foreach my $q (@{$vorlist->{$tf}}) {
   printf("\tf: %f, scan_dB: %f, snr_db: %f\n",$q->{f},$q->{scan_dB},$q->{snr_dB});
  }
 };
 print "=== VOR scan complete ===\n";
 $s->{last}{vor_scan} = $vorlist;
 return $vorlist;
};



sub idVORs {
 my $s = shift;
 my $vorlist = undef;
 if (defined($s->{last}{vor_scan})) {
  $vorlist = $s->{last}{vor_scan};
 } else {
  print "-pe- cannot call idVORs before findVORs\n";
  return undef;
 }

 print "=== Start ID Scan Starting ===\n";
 my $id_list = {};
 foreach my $true_f (keys %$vorlist) {
  my @fs = sort { $b->{scan_dB} <=> $a->{scan_dB} } @{$vorlist->{$true_f}};
  my $best = $fs[0];

  change_freq($s->{cfg},$best->{f});
  usleep($s->{scfg}{decode}{id_pre_dwell});
  my $idstr = '';
  my $r;
  print "id_instr: ";
  my $start = time();
  while (time() < ($start + ($s->{scfg}{decode}{id_dwell}/1e6))) {
   $r = try_pop_all($s->{cfg}{rstat_queue});
   if ($r->{have_status} && $r->{have_carrier}) {
    $idstr .= $r->{decoded_str};
    #print "idstr: $idstr\n";
   }
  }
  print "\n";
  my @chunks = split(/\s/,$idstr);
  foreach my $chunk (@chunks) {
   if ($chunk =~ /[A-Z]{3}/) {
    my $id = $chunk;
    my $db_freq = $s->{vordata}{$id}{frequency};
    if ($true_f == $db_freq) {
     $id_list->{$true_f} = {
      id => $id,
      f => $best->{f},
      true_f => $true_f,
      scan_dB => $best->{scan_dB},
      snr_dB => $r->{strength_ratio_lpf},
     };
    } else {
     print "-pd- did not add $id because freq $true_f does not match database $db_freq\n";
    }
   }
  };
 };

 $s->{last}{id_scan} = $id_list;

 print "=== ID Scan complete ===\n";

 return $id_list;
};


sub deg2dms {
 my $d = shift;
 my $neg = 0;
 if ($d < 0) {
  $d = abs($d); $neg = 1;
 }
 my $d_o = int($d);
 $d   -= $d_o;
 my $m_o = int($d * 60);
 $d   -= ($m_o/60);
 my $s_o = int($d * 3600);
 $d   -= ($s_o/3600);
 if ($neg) { $d_o = -$d_o; };
 return ($d_o,$m_o,$s_o);
}

sub calcPosition {
 my $s = shift;
 my $idlist = undef;
 if (defined($s->{last}{id_scan})) {
  $idlist = $s->{last}{id_scan};
 } else {
  print "-pe- cannot call calPosition before idScan\n";
  return undef;
 }

 my $radials = {};
 foreach my $t_f (keys %$idlist) {
  my $f = $idlist->{$t_f}{f};
  change_freq($s->{cfg},$f);
  usleep ($s->{scfg}{decode}{pos_dwell});
  my $r = try_pop_all($s->{cfg}{rstat_queue});
  if ($r->{have_status} && $r->{have_carrier}) {
   my $radial = $r->{radial_lpf};
   my $id     = $idlist->{$t_f}{id};
   $radials->{$id} = {
    radial_m => $radial,
    id       => $id,
    lat      => $s->{vordata}{$id}{latitude},
    lon      => $s->{vordata}{$id}{longitude},
    m_var    => $s->{vordata}{$id}{magnetic_variation},
    radial_t => $radial + $s->{vordata}{$id}{magnetic_variation},
   }
  } else {
   # should not be necessary because inability to generate fix 
   # will force re-scan for receivable VORs
   # delete $idlist->{$t_f};
  }
 }

 printf("-Radials- id   lat          lon         mvar     radial (m)\n");
 foreach my $id (keys %$radials) {
  printf("          %.3s %3.5f %3.5f %2.2f %3.2f\n", 
	  $id,
	  $radials->{$id}{lat},
	  $radials->{$id}{lon},
	  $radials->{$id}{m_var},
	  $radials->{$id}{radial_m});
 };

 my $r_count = scalar keys %$radials;

 my $positions = {};
 foreach my $r1k (keys %$radials) { 
  foreach my $r2k (keys %$radials) {
   if ($r1k ne $r2k) {
    my $name = $r1k . '_' . $r2k;
    my $revname = $r2k . '_' . $r1k;
    if (!defined($positions->{$revname})) {
     my $r1 = $radials->{$r1k};
     my $r2 = $radials->{$r2k};
     my $pos = triangCartesian(
	     { x => $r1->{lon}, y => $r1->{lat}, r => $r1->{radial_t}},
 	     { x => $r2->{lon}, y => $r2->{lat}, r => $r2->{radial_t}});
     if (defined($pos->{solution}) && $pos->{solution}) {
      $positions->{$name} = {
       lon => $pos->{x},
       lat => $pos->{y}
      };
     }
    }
   }
  }
 };

 my $best_guess = { lat => 0, lon => 0, count => 0 };
 map { $best_guess->{lat} += $_->{lat};
       $best_guess->{lon} += $_->{lon};
       $best_guess->{count}++; } (values %$positions);
 if ($best_guess->{count}) {
  $best_guess->{lat} /= $best_guess->{count};
  $best_guess->{lon} /= $best_guess->{count};
 };

 return {
  have_position => ($best_guess->{count} > 0),
  positions => $positions,
  best_guess => $best_guess,
 };
};


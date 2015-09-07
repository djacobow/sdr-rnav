#!/usr/bin/perl -w

use warnings qw(all);
use strict;
use Data::Dumper;

use djdsp;
use Time::HiRes;
use YAML qw(LoadFile);

$| = 1;

my $cfg = {
 indir => "..\\..\\test_input_files\\",
 odir  => "..\\..\\test_output_files\\",
 infn  => "OAK_r324_20130429_162637Z_116782kHz_250000sps_IQ.wav",
 vordb => "..\\src\\vor_db\\vordb.yaml",

 scan => {
  sr => 1024000,
  vor_f_range => [ 108000000, 117950000 ],
  min_dwell => 2,
  max_dwell => 3,
  max_per_hop => 5,
  peaks_thresh => 20,
 },
 decode => {
  calibrate_radial => 195,
  sr => 250000,
  id_dwell => 30,
  min_dwell => 2,
 },
};

if ($^O =~ /linux/) { 
 foreach my $x ('indir', 'odir', 'vordb') {
  $cfg->{$x} =~ s/\\/\//g; 
 }
};

my $vor_db = LoadFile($cfg->{vordb});

radio_init(0);
radio_preset_max_cap_seconds(600);
radio_preset_infile($cfg->{indir} . $cfg->{infn});
radio_preset_outdir($cfg->{odir});
radio_preset_peaks_thresh($cfg->{scan}{peaks_thresh});

radio_dsp_init();
radio_calibrate_radial($cfg->{decode}{calibrate_radial});

# must give the radio a full second to get started. rtl-sdr bug
# this is not necessary (or desirable) when working from a file 
# as the decoder will zip through the file in non-real-time
sleep 1; 


radio_enable_mixer(0);
radio_set_sample_rate($cfg->{decode}{sr});
radio_set_frequency(116795448+0);
radio_set_tuner_gain_mode(0);
radio_enable_agc(1);
radio_enable_fft(0);

while (!radio_dsp_all_done()) {
 my $potential_signals = findSignals();

 my $probable_vors     = findVORs($potential_signals);

 my $id_vors           = idVORs($probable_vors);
 print Dumper $id_vors;


};

radio_shutdown();


sub findSignals {

 print "==== Starting signal scan ====\n";

 my $interesting_sigs = {};

 radio_enable_fft(1);
 radio_set_sample_rate($cfg->{scan}{sr});

 my $f_start = int($cfg->{scan}{vor_f_range}[0] + ($cfg->{scan}{sr} / 2));
 my $f_end   = int($cfg->{scan}{vor_f_range}[1] - ($cfg->{scan}{sr} / 2));

 while ($f_start < $f_end) {
  radio_set_frequency($f_start);
  sleep($cfg->{scan}{min_dwell});
  my $start_time = time();
  my $done = 0;
  while (!$done) {
   my $s = radio_get_fft();
   if (defined($s) && ($s->{have_fft})) {
    my @candidates = sort { $b->{dB} <=> $a->{dB} } @{$s->{points}};
    my $ccount = scalar @candidates;
    $ccount = ($ccount > $cfg->{scan}{max_per_hop}) ? $cfg->{scan}{max_per_hop} : $ccount;
    for (my $i=0;$i<$ccount;$i++) {
     $interesting_sigs->{ $candidates[$i]->{f} } = $candidates[$i];
    }
    $done = 1;
   } elsif (time() > ($cfg->{scan}{max_dwell} + $start_time)) {
    $done = 1;
   }
  }
  $f_start += $cfg->{scan}{sr};
 };

 radio_set_sample_rate($cfg->{decode}{sr});
 radio_enable_fft(0);

 foreach my $f (keys %$interesting_sigs) {
  printf("signal: f=%f scan_snr=%f\n",$f,$interesting_sigs->{$f}{dB});
 }

 print "==== signal scan complete ====\n";

 return $interesting_sigs;
};


sub findVORs {
 my $siglist = shift;
 my $vorlist = {};

 print "=== Starting VOR scan ===\n";

 my @vors_in_order = sort { $siglist->{$b}{dB} <=> $siglist->{$a}{dB} } keys %$siglist;

 my $ct = 0;
 foreach my $f (@vors_in_order) {
  radio_set_frequency($f);
  sleep ($cfg->{decode}{min_dwell});
  # frequency spacing for VORs is 50kHz
  my $true_f = 50000 * int($f / 50000 + 0.5);
  my $done = 0;
  while (!$done) {
   my $s = radio_get_status();
   if ($s->{have_status}) {
    $done = 1;
    if ($s->{have_carrier} && $ct < 20) {
     if (!defined($vorlist->{$true_f})) {
      $vorlist->{$true_f} = [];
     }
     push(@{$vorlist->{$true_f}}, 
      {
       f => $f,
       true_f => $true_f,
       scan_dB => $siglist->{$f}{dB},
       snr_dB => $s->{strength_ratio_lpf},
      });
     printf("looks_like_vor f:%f, true_f:%f, search dB:%f, snr:%f\n",$f,$true_f,$siglist->{$f}{dB},$s->{strength_ratio_lpf});
     $ct++;
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

 return $vorlist;
};


sub idVORs {
 my $vorlist = shift;
 print "=== Start ID Scan complete ===\n";
 my $id_list = {};
 foreach my $true_f (keys %$vorlist) {
  my @fs = sort { $b->{scan_dB} <=> $a->{scan_dB} } @{$vorlist->{$true_f}};
  my $best = $fs[0];

  radio_set_frequency($best->{f});
  my $idstr = '';
  my $start = time();
  my $s;
  print "id_instr: ";
  while (time() < ($start + $cfg->{decode}{id_dwell})) {
   $s = radio_get_status();
   if ($s->{have_status} && $s->{have_carrier}) {
    $idstr .= $s->{id_instr};
    print $s->{id_instr};
   }
  }
  print "\n";
  my @chunks = split(/\s/,$idstr);
  foreach my $chunk (@chunks) {
   if ($chunk =~ /[A-Z]{3}/) {
    my $id = $chunk;
    $id_list->{$true_f} = {
     id => $id,
     f => $best->{f},
     scan_dB => $best->{scan_dB},
     snr_dB => $s->{strength_ratio_lpf},
    };
   }
  };
 };

 print "=== ID Scan complete ===\n";

 return $id_list;
};


__END__

radio_enable_mixer(0);
sleep(1);
radio_enable_mixer(1);
sleep(1);
radio_enable_mixer(0);
#$djdsp::_main_perform_fft = 0x1;
my $end = time + 60;
while (time < $end) {
 my $status = radio_get_status();
 print Dumper $status;
 my $fstat  = radio_get_fft();
 if (defined($fstat) && ($fstat->{have_fft})) {
 }
 print Dumper $fstat;
 sleep 1;
}


=cut
while (!radio_dsp_all_done()) {
 my $fstat = radio_get_fft();
 if (defined($fstat) && ($fstat->{have_fft})) {
  print Dumper $fstat;
 } else {
  my $status = radio_get_status();
  if ($status->{have_status}) {
   if ($status->{have_carrier} && length($status->{id_instr})) {
    printf "lpf radial: " . $status->{angle_lpf} . " ident: \"" . 
           $status->{id_instr} . "\"\n";
   }
  }
 }
};
=cut


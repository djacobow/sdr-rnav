#!/usr/bin/perl -w

use warnings qw(all);
use strict;
use threads;
use Data::Dumper;
use dsp_chain;
use dplib;
use Time::HiRes qw(usleep);
use sl_getter;
use rcontrol;
use vorscan;
use YAML qw(LoadFile);

$| = 1;

my $cfg = LoadFile('basic_config.yaml');

fixFilePaths($cfg->{files});

my $sl  = dplib::set_sl(dp_pl_sharelist_create(30));
my $slg = sl_getter->new($sl);
$cfg->{sl}  = $sl;
$cfg->{slg} = $slg;

# perl doesn't know how to treat values loaded from
# YAML file. This gives a strong hint that these are
# to be treated as integers.
foreach my $mn (keys %{$cfg->{rmasks}}) {
 $cfg->{rmasks}{$mn} += 0;
}

my $starting_rmask = 
	($cfg->{rmasks}{DECODE} + 0)|
        ($cfg->{rmasks}{MIXER} + 0) |
	($cfg->{rmasks}{ALWAYS} + 0);

$slg->add_u32('runmask',$starting_rmask);
$slg->add_u32('dsp_done',0);
$slg->add_u32('use_radio',$cfg->{use_radio});
$slg->add_u32('perform_fft',0);
$slg->add_i32('mixer_lo_freq',5100);
if ($cfg->{use_radio}) {
 $slg->add_u32('use_mixer',0);
} else {
 $slg->add_u32('use_mixer',1);
}
$slg->add_u32('curr_freq',88500000);
$slg->add_u32('curr_sample_rate',$cfg->{srates}{if0});


my ($radio, $cb_thread) = (undef, undef);
if ($cfg->{use_radio}) {
 ($radio, $cb_thread) = radio_init(0);
}
my $olist = createDSPchain($cfg,$radio);

updateSettings($cfg);

my $dsp_thread = threads->create(\&dsp_thread,$cfg);

if (!$cfg->{use_radio}) {
 # fft_mode_enable($cfg,1);
 while (!$slg->get_u32('dsp_done')) {
  # print Dumper try_pop_all($cfg->{fstat_queue});
  usleep(100000);
 }
} else {

 my $rq = $cfg->{rstat_queue};
 my $fq = $cfg->{fstat_queue};

 my $vscan = vorscan->new($cfg);

 my $have_pos = 0;
 while (!$slg->get_u32('dsp_done')) {
  if (!$have_pos) {
   my $signals = $vscan->findSignals();
   print Dumper $signals;
   my $vors    = $vscan->findVORs();
   print Dumper $vors;
   my $id_vors = $vscan->idVORs();
   print Dumper $id_vors;
  }
  my $pos = $vscan->calcPosition();
  if ($pos->{have_position}) {
   print Dumper $pos;
  };
 };

 radioShutdown($radio);
 $cb_thread->join();
}

dp_pl_sharelist_debug($sl); 

$dsp_thread->join();


# ############################################################
# ## subroutines 
# ############################################################


sub dsp_thread {
 my $cfg = shift;
 my $olist = $cfg->{olist};
 dplib::dsp_thread_fn($cfg,$olist->{bl});
};

sub fixFilePaths {
 my $files = shift;
 foreach my $dir ('in', 'out') {
  my $repl = $files->{$dir . 'dir'};
  foreach my $fn (keys %{$files->{$dir}}) {
   $files->{$dir}{$fn} = $repl . $files->{$dir}{$fn};
   if ($^O eq 'MSWin32') {
    $files->{$dir}{$fn} =~ s/\//\\/g;
   }
  }
 }
}

sub perl_dsp_after_cb {
 dsp_chain::perl_dsp_after_cb(@_);
};


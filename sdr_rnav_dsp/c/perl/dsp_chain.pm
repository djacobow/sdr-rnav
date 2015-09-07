#!/usr/bin/perl -w
#
# Author: David Jacobowitz
#         david.jacobowitz@gmail.com
#
# Date  : Spring, Summer, Fall 2013
#
# Copyright 2013, David Jacobowitz


# routines to set up a DSP chain, using the dplib wrapper to the 
# dp DSP objects, that can decode VOR signals

package dsp_chain;

use strict;
use warnings qw(all);

use threads;
use Exporter qw(import);
use Data::Dumper;
use Cwd qw(getcwd);
use dplib;
use firdes;
use Thread::Queue;
use Scalar::Util qw(looks_like_number);


our @EXPORT = qw(radioShutdown updateSettings createDSPchain perl_dsp_cb);

# scan this file and make filters;
my $filters = firdes(files => [ getcwd() . '/' . 'filters.yaml' ], 
                     embedded => 0 );


sub radioShutdown {
 my $r = shift;
 dp_radio2832_close_device($r);
};

sub addObj {
 my $olist = shift;
 my $name  = shift;
 my $obj   = shift;
 my $grp   = shift || 0;
 $olist->{named}{$name} = { 
  obj => $obj,
 };
 dp_baselist_add($olist->{bl},$obj);
 dp_set_name($obj,$name);
 dp_set_group($obj,$grp);
 return $obj;
};

sub updateSettings {
 my $cfg       = shift;

 my $sl       = $cfg->{sl};
 my $olist    = $cfg->{olist};
 my $buffers  = $cfg->{buffers};
 my $slg      = $cfg->{slg};

 my $mixer = $olist->{named}{mixer}{obj};
 my $radio = $olist->{named}{radio0}{obj};
 my $lp    = $olist->{named}{if1_lpfd}{obj};

 my $do_fft    = $slg->get_u32('perform_fft');
 my $use_mixer = $slg->get_u32('use_mixer');
 my $runmask   = $slg->get_u32('runmask');
 my $lo_freq   = $slg->get_i32('mixer_lo_freq');
 my $c_freq    = $slg->get_u32('curr_freq');
 my $c_sr      = $slg->get_u32('curr_sample_rate');

 # will let the top level routines decide when to change
 # frequency directly, but this one will adjust signal chain 
 # and change sample rate.
 if (1) {
  if ($cfg->{use_radio}) {
#   dp_radio2832_dev_cmd_st_only($radio,$cfg->{rcmds}{ST_FREQ},$c_freq);
   dp_radio2832_dev_cmd_st_only($radio,$cfg->{rcmds}{ST_SR},$c_sr);
  }
 }
 my $mix = $use_mixer && !dp_cmix_set_lo_freq($mixer,$lo_freq);

 if ($do_fft) {
  print "-pi- fft scan on\n";
  $runmask |= $cfg->{rmasks}{FFT};
  $runmask &= ~$cfg->{rmasks}{DECODE};
  $runmask &= ~$cfg->{rmasks}{MIXER};
 } elsif ($mix) {
  print "-pi- decode/mix on\n";
  $runmask &= ~$cfg->{rmasks}{FFT};
  $runmask |= $cfg->{rmasks}{DECODE};
  $runmask |= $cfg->{rmasks}{MIXER};
  dp_set_in($lp,$buffers->{v_if1_c_mixed});
 } else {
  print "-pi- decode/nomix on\n";
  $runmask &= ~$cfg->{rmasks}{FFT};
  $runmask |= ~$cfg->{rmasks}{DECODE};
  $runmask &= ~$cfg->{rmasks}{MIXER};
  dp_set_in($lp,$buffers->{v_if1_c_read});
 }
 $slg->set_u32('runmask',$runmask);
};

sub createDSPchain {
 my $cfg   = shift;
 my $radio = shift;

 my $sl  = $cfg->{sl};
 my $slg = $cfg->{slg};

 my $power_ratio = 0;

 my $real_sr;
 my $var30_decim;
 my $ref30_decim,
 my $start;
 my $buffer_ct;
 my $iter_decoded_str = '';

 $cfg->{blens}{if1} =
  $cfg->{blens}{if0} /
   ($cfg->{srates}{if0} /
    $cfg->{srates}{if1});

 $cfg->{blens}{af} =
  $cfg->{blens}{if1} / 
   ($cfg->{srates}{if1} /
    $cfg->{srates}{af});

 $cfg->{blens}{nf} =
  $cfg->{blens}{af} / 
   ($cfg->{srates}{af} /
    $cfg->{srates}{nf});

 $cfg->{blens}{mo} =
  $cfg->{blens}{af} / 
   ($cfg->{srates}{af} /
    $cfg->{srates}{mo});


# print Dumper $cfg->{blens}; 
# print Dumper $cfg->{srates};

 my $buffers = {
  v_if0_c_read            => dp_vec_create($cfg->{blens}{if0}*2),
  v_if0_c_filt            => dp_vec_create($cfg->{blens}{if0}*2),
  v_if1_c_read            => dp_vec_create($cfg->{blens}{if1}*2),
  v_if0_c_fft             => dp_vec_create($cfg->{blens}{if0}*2),
  v_if0_fft_mag           => dp_vec_create($cfg->{blens}{if0}),
  v_if1_c_mixed           => dp_vec_create($cfg->{blens}{if1}*2),
  v_if1_c_lpfd_bb         => dp_vec_create($cfg->{blens}{if1}*2),
  v_af_c_decim_bb         => dp_vec_create($cfg->{blens}{af}*2),
  v_af_bb                 => dp_vec_create($cfg->{blens}{af}),
  v_af_unfiltered_audio   => dp_vec_create($cfg->{blens}{af}),
  v_af_nodc_audio         => dp_vec_create($cfg->{blens}{af}),
  v_af_agcd_audio         => dp_vec_create($cfg->{blens}{af}),
  v_af_var30_pass1        => dp_vec_create($cfg->{blens}{af}),
  v_af_var30_decim        => dp_vec_create($cfg->{blens}{nf}),
  v_nf_var30              => dp_vec_create($cfg->{blens}{nf}),
  v_nf_var100             => dp_vec_create($cfg->{blens}{nf}),
  v_af_fmcarrier          => dp_vec_create($cfg->{blens}{af}),
  v_af_ref30_1            => dp_vec_create($cfg->{blens}{af}),
  v_nf_ref30_decim        => dp_vec_create($cfg->{blens}{nf}),
  v_nf_ref30              => dp_vec_create($cfg->{blens}{nf}),
  v_mo_morse_audio_unf    => dp_vec_create($cfg->{blens}{mo}),
  v_mo_morse_audio        => dp_vec_create($cfg->{blens}{mo}),
  v_mo_morse_env          => dp_vec_create($cfg->{blens}{mo}),
  v_mo_morse_binary       => dp_vec_create($cfg->{blens}{mo}), 
  # actually much smaller
  v_af_ditdah             => dp_vec_create($cfg->{blens}{af}), 
 };
 $cfg->{buffers} = $buffers;

 my $olist = {
  named => {},
  bl => dp_baselist_create($cfg->{max_baselist}),
 };
 $cfg->{olist} = $olist;


 my $o = undef;
 if ($cfg->{use_radio}) {
  $o = addObj($olist,'radio0',$radio,$cfg->{rmasks}{ALWAYS});
  dp_radio2832_set_buffers($o,$cfg->{blens}{if0},$cfg->{rtl_buf_ct});
  dp_set_out($o,$buffers->{v_if0_c_read});
  my $freq     = $slg->get_u32('curr_freq');
  my $sr       = $slg->get_u32('curr_sample_rate');
  dp_radio2832_dev_cmd_st_only($o,$cfg->{rcmds}{ST_FREQ},$freq);
  dp_radio2832_dev_cmd_st_only($o,$cfg->{rcmds}{ST_SR},$sr);
  dp_radio2832_dev_cmd_st_only($o,$cfg->{rcmds}{ST_TUNER_GAIN_MODE},0);
  dp_radio2832_dev_cmd_st_only($o,$cfg->{rcmds}{ST_AGC},1);
  #dp_radio2832_dev_cmd_st_only($o,$cfg->{rcmds}{ST_TEST},1);
 } else {
  $o = addObj($olist,'reader',dp_create_wav_r(),$cfg->{rmasks}{ALWAYS});
  print "o $o\n";
  dp_set_out($o,$buffers->{v_if0_c_read});
  dp_set_runlen($o,$cfg->{blens}{if0});
  dp_wav_r_set_fname($o,$cfg->{files}{in}{iq});
 };


 $o = addObj($olist,'scan_fft',dp_create_cfft(),$cfg->{rmasks}{FFT});
 dp_cfft_set_averaging($o,0.8);
 dp_set_inlt($o,$buffers->{v_if0_c_read},$cfg->{blens}{if0},1);
 dp_set_out($o,$buffers->{v_if0_c_fft});

 if ($cfg->{debug}) {
  $o = addObj($olist,'scan_writer',dp_create_wav_w(),$cfg->{rmasks}{FFT});
  dp_wav_w_set_num_channels($o,2);
  dp_wav_w_set_sample_rate($o,$cfg->{srates}{if0});
  dp_wav_w_set_bits_per_sample($o,16);
  dp_set_inl($o,$buffers->{v_if0_c_read},$cfg->{blens}{if0});
  dp_wav_w_set_fname($o,$cfg->{files}{out}{fft_unf});
 }


 $o = addObj($olist,'fft_c2r',dp_create_c2r(),$cfg->{rmasks}{FFT});
 dp_set_runlen($o,$cfg->{blens}{if0});
 dp_set_inlt($o,$buffers->{v_if0_c_fft},$cfg->{blens}{if0},1);
 dp_set_out($o,$buffers->{v_if0_fft_mag});


 $o = addObj($olist,'fft_peaks',dp_create_findpeaks(),$cfg->{rmasks}{FFT});
 dp_set_inl($o,$buffers->{v_if0_fft_mag},$cfg->{blens}{if0});
 my $ppk_pts = dp_pl_alloc_peak_pts_t();
 dplib::peak_pts_t_init($ppk_pts);
 dp_pl_sharelist_add_vp($sl,'dsp_ppeak_pts',$ppk_pts);
 dp_findpeaks_set_out($o,$ppk_pts);
 dp_findpeaks_set_threshold($o,$cfg->{fft_pk_thresh});

 $o = setupFilter($cfg,'lp100k_sr1024k','if0_if1_prefilt');
 dp_set_inlt($o,$buffers->{v_if0_c_read},$cfg->{blens}{if0},1);
 dp_set_out($o,$buffers->{v_if0_c_filt});
 
 if ($cfg->{debug}) {
  $o = addObj($olist,'if0_c_filt_writer',dp_create_wav_w(),$cfg->{rmasks}{DECODE});
  dp_wav_w_set_num_channels($o,2);
  dp_wav_w_set_sample_rate($o,$cfg->{srates}{if0});
  dp_wav_w_set_bits_per_sample($o,16);
  dp_set_inl($o,$buffers->{v_if0_c_filt},$cfg->{blens}{if0});
  dp_wav_w_set_fname($o,$cfg->{files}{out}{if0_c_filt});
 }

 $o = addObj($olist,'if0_if1_decimator',dp_create_decimate(),$cfg->{rmasks}{DECODE});
 dp_decimate_set_decim($o,$cfg->{srates}{if0}/$cfg->{srates}{if1});
 dp_set_inlt($o,$buffers->{v_if0_c_filt}, $cfg->{blens}{if0},1);
 dp_set_out($o,$buffers->{v_if1_c_read});

 if ($cfg->{debug}) {
  $o = addObj($olist,'if1_premix_writer',dp_create_wav_w(),$cfg->{rmasks}{DECODE});
  dp_wav_w_set_num_channels($o,2);
  dp_wav_w_set_sample_rate($o,$cfg->{srates}{if1});
  dp_wav_w_set_bits_per_sample($o,16);
  dp_set_inl($o,$buffers->{v_if1_c_read},$cfg->{blens}{if1});
  dp_wav_w_set_fname($o,$cfg->{files}{out}{if1_c_read_copy});
 }

 $o = addObj($olist,'mixer',dp_create_cmix(),$cfg->{rmasks}{MIXER});
 dp_cmix_set_sample_rate($o,$cfg->{srates}{if1});
 dp_set_inlt($o,$buffers->{v_if1_c_read},$cfg->{blens}{if1},1);
 dp_set_out($o,$buffers->{v_if1_c_mixed});
 dp_cmix_set_lo_freq($o,$cfg->{slg}->get_u32('mixer_lo_freq'));

 if ($cfg->{debug}) {
  $o = addObj($olist,'if1_mixed_writer',dp_create_wav_w(),$cfg->{rmasks}{DECODE});
  dp_wav_w_set_num_channels($o,2);
  dp_wav_w_set_sample_rate($o,$cfg->{srates}{if1});
  dp_wav_w_set_bits_per_sample($o,16);
  dp_set_inl($o,$buffers->{v_if1_c_mixed},$cfg->{blens}{if1});
  dp_wav_w_set_fname($o,$cfg->{files}{out}{if1_c_mixed});
 }

 $o = setupFilter($cfg,'lp25k_sr256k','if1_lpfd');
 dp_set_inlt($o,$buffers->{v_if1_c_mixed},$cfg->{blens}{if1},1);
 dp_set_out($o,$buffers->{v_if1_c_lpfd_bb});


 $o = addObj($olist,'if1_af_decimator',dp_create_decimate(),$cfg->{rmasks}{DECODE});
 dp_decimate_set_decim($o,$cfg->{srates}{if1}/$cfg->{srates}{af});
 dp_set_inlt($o,$buffers->{v_if1_c_lpfd_bb},$cfg->{blens}{if1},1);
 dp_set_out($o,$buffers->{v_af_c_decim_bb});


 if ($cfg->{debug}) {
  if (0) {
   $o = addObj($olist,'if1_mixed_writer',dp_create_wav_w(),$cfg->{rmasks}{DECODE});
   dp_wav_w_set_num_channels($o,2);
   dp_wav_w_set_sample_rate($o,$cfg->{srates}{if1});
   dp_wav_w_set_bits_per_sample($o,16);
   dp_set_inl($o,$buffers->{v_if1_c_mixed},$cfg->{blens}{if1});
   dp_wav_w_set_fname($o,$cfg->{files}{out}{if1_mixed});
  }

  $o = addObj($olist,'af_bb_writer',dp_create_wav_w(),$cfg->{rmasks}{DECODE});
  dp_wav_w_set_num_channels($o,2);
  dp_wav_w_set_sample_rate($o,$cfg->{srates}{af});
  dp_wav_w_set_bits_per_sample($o,16);
  dp_set_inl($o,$buffers->{v_af_c_decim_bb},$cfg->{blens}{af});
  dp_wav_w_set_fname($o,$cfg->{files}{out}{af_bb});
 };


 $o = addObj($olist,'af_bb_c2r',dp_create_c2r(),$cfg->{rmasks}{DECODE});
 dp_set_inlt($o,$buffers->{v_af_c_decim_bb},$cfg->{blens}{af},1);
 dp_set_out($o,$buffers->{v_af_bb});


 $o = addObj($olist,'af_bb_am_demod',dp_create_envelope(),$cfg->{rmasks}{DECODE});
 dp_envelope_set_sample_rate($o,$cfg->{srates}{af});
 dp_envelope_set_attack_s($o,0.0002);
 dp_envelope_set_release_s($o,0.0002);
 dp_set_inl($o,$buffers->{v_af_bb},$cfg->{blens}{af});
 dp_set_out($o,$buffers->{v_af_unfiltered_audio});


 $o = addObj($olist,'af_bb_dcblock',dp_create_dcblock(),$cfg->{rmasks}{DECODE});
 dp_dcblock_set_pole($o,0.99999); # current implementation does not use
 dp_set_inl($o,$buffers->{v_af_unfiltered_audio},$cfg->{blens}{af});
 dp_set_out($o,$buffers->{v_af_nodc_audio});

 $o = addObj($olist,'af_agc',dp_create_agc(),$cfg->{rmasks}{DECODE});
 dp_agc_set_time_constant($o,0.8);
 dp_agc_set_desired_mag($o,40);
 dp_set_inl($o,$buffers->{v_af_nodc_audio},$cfg->{blens}{af});
 dp_set_out($o,$buffers->{v_af_agcd_audio});


 if ($cfg->{debug}) {
  $o = addObj($olist,'af_audio_writer',dp_create_wav_w(),$cfg->{rmasks}{DECODE});
  dp_wav_w_set_num_channels($o,1);
  dp_wav_w_set_sample_rate($o,$cfg->{srates}{af});
  dp_wav_w_set_bits_per_sample($o,16);
  dp_wav_w_set_fname($o,$cfg->{files}{out}{af_audio});
  dp_set_inl($o,$buffers->{v_af_agcd_audio},$cfg->{blens}{af});
 };


 if ($cfg->{linux_audio}) {
  $o = addObj($olist,'af_audio_output',dp_create_linaudio(),$cfg->{rmasks}{DECODE});
  dp_linaudio_set_num_channels($o,1);
  dp_linaudio_set_sample_rate($o,$cfg->{srates}{af});
  dp_linaudio_set_bits_per_sample($o,16);
  dp_set_inl($o,$buffers->{v_af_agcd_audio},$cfg->{blens}{af});
 };


 $o = setupFilter($cfg,'lp400_sr32000','af_lp400');
 dp_set_inlt($o,$buffers->{v_af_agcd_audio},$cfg->{blens}{af},0);
 dp_set_out($o,$buffers->{v_af_var30_pass1});


 $o = addObj($olist,'af_nf_decimator',dp_create_decimate(),$cfg->{rmasks}{DECODE});
 dp_decimate_set_decim($o,$cfg->{srates}{af}/$cfg->{srates}{nf});
 dp_set_inlt($o,$buffers->{v_af_var30_pass1},$cfg->{blens}{af},0);
 dp_set_out($o,$buffers->{v_af_var30_decim});


 if (0) {
  $o = addObj($olist,'nf_lp400_writer',dp_create_wav_w(),$cfg->{rmasks}{DECODE});
  dp_wav_w_set_num_channels($o,1);
  dp_wav_w_set_sample_rate($o,$cfg->{srates}{nf});
  dp_wav_w_set_bits_per_sample($o,16);
  dp_wav_w_set_fname($o,$cfg->{files}{out}{nf_lp400});
  dp_set_inl($o,$buffers->{v_af_var30_decim},$cfg->{blens}{nf});
 };


 $o = setupFilter($cfg,'bp30_sr1280','nf_bp30');
 dp_set_inlt($o,$buffers->{v_af_var30_decim},$cfg->{blens}{nf},0);
 dp_set_out($o,$buffers->{v_nf_var30});


 if ($cfg->{debug}) {
  $o = addObj($olist,'nf_var30_text_writer',dp_create_text(),$cfg->{rmasks}{DECODE});
  dp_text_set_header($o,'var30');
  dp_text_set_fname($o,$cfg->{files}{out}{nf_var30text});
  dp_text_set_channels($o,1);
  dp_set_inl($o,$buffers->{v_nf_var30},$cfg->{blens}{nf});


  $o = addObj($olist,'nf_var30_writer',dp_create_wav_w(),$cfg->{rmasks}{DECODE});
  dp_wav_w_set_num_channels($o,1);
  dp_wav_w_set_sample_rate($o,$cfg->{srates}{nf});
  dp_wav_w_set_bits_per_sample($o,16);
  dp_set_inl($o,$buffers->{v_nf_var30},$cfg->{blens}{nf});
  dp_wav_w_set_fname($o,$cfg->{files}{out}{nf_var30});
 }

 $o = setupFilter($cfg,'bp105_sr1280','nf_bpf105');
 dp_set_inlt($o,$buffers->{v_af_var30_decim},$cfg->{blens}{nf},0);
 dp_set_out($o,$buffers->{v_nf_var100});


 if ($cfg->{debug}) {
  $o = addObj($olist,'nf_var100_text_writer',dp_create_text(),$cfg->{rmasks}{DECODE});
  dp_text_set_fname($o,$cfg->{files}{out}{nf_var100text});
  dp_text_set_channels($o,1);
  dp_text_set_header($o,'var100');
  dp_set_inl($o,$buffers->{v_nf_var100},$cfg->{blens}{nf});
 } 


 $o = addObj($olist,'nf_var30_var100_compare',dp_create_ecompare(),$cfg->{rmasks}{DECODE});
 dp_set_runlen($o,$cfg->{blens}{nf});
 dp_ecompare_set_ins($o,$buffers->{v_nf_var30},$buffers->{v_nf_var100});
 my $ppower_ratio = dp_pl_alloc_float();
 dp_pl_sharelist_add_fp($sl,'dsp_ppower_ratio',$ppower_ratio);
 dp_ecompare_set_out($o,$ppower_ratio);


 $o = setupFilter($cfg,'bp9960_sr32000','af_bpf9960');
 dp_set_inlt($o,$buffers->{v_af_bb},$cfg->{blens}{af},0);
 dp_set_out($o,$buffers->{v_af_fmcarrier});


 if ($cfg->{debug}) {
  $o = addObj($olist,'c9960_writer',dp_create_wav_w(),$cfg->{rmasks}{DECODE});
  dp_wav_w_set_num_channels($o,1);
  dp_wav_w_set_sample_rate($o,$cfg->{srates}{af});
  dp_wav_w_set_bits_per_sample($o,16);
  dp_wav_w_set_fname($o,$cfg->{files}{out}{af_9960});
  dp_set_inl($o,$buffers->{v_af_fmcarrier},$cfg->{blens}{af});

  $o = addObj($olist,'c9960_writer2',dp_create_wav_w(),$cfg->{rmasks}{DECODE});
  dp_wav_w_set_num_channels($o,1);
  dp_wav_w_set_sample_rate($o,$cfg->{srates}{af});
  dp_wav_w_set_bits_per_sample($o,16);
  dp_wav_w_set_fname($o,$cfg->{files}{out}{af_9960_2});
  dp_set_inl($o,$buffers->{v_af_bb},$cfg->{blens}{af});
 }


 $o = addObj($olist,'fm_demod',dp_create_quasiquad(),$cfg->{rmasks}{DECODE});
 dp_quasiquad_setup($o,$cfg->{srates}{af},$cfg->{vor_parameters}{subcarrier_freq},
	 $cfg->{vor_parameters}{subcarrier_max_dev});
 dp_set_inl($o,$buffers->{v_af_fmcarrier},$cfg->{blens}{af});
 dp_set_out($o,$buffers->{v_af_ref30_1});


 $o = addObj($olist,'ref30_decimator',dp_create_decimate(),$cfg->{rmasks}{DECODE});
 dp_decimate_set_decim($o,$cfg->{srates}{af}/$cfg->{srates}{nf});
 dp_set_inlt($o,$buffers->{v_af_ref30_1},$cfg->{blens}{af},0);
 dp_set_out($o,$buffers->{v_nf_ref30_decim});


 $o = setupFilter($cfg,'bp30_sr1280','nf_bpf_ref30');
 dp_set_inlt($o,$buffers->{v_nf_ref30_decim},$cfg->{blens}{nf},0);
 dp_set_out($o,$buffers->{v_nf_ref30});


 if ($cfg->{debug}) {
  $o = addObj($olist,'nf_ref30_writer',dp_create_wav_w(),$cfg->{rmasks}{DECODE});
  dp_wav_w_set_num_channels($o,1);
  dp_wav_w_set_sample_rate($o,$cfg->{srates}{nf});
  dp_wav_w_set_bits_per_sample($o,16);
  dp_set_inl($o,$buffers->{v_nf_ref30},$cfg->{blens}{nf});
  dp_wav_w_set_fname($o,$cfg->{files}{out}{nf_ref30});


  $o = addObj($olist,'nf_ref30_text',dp_create_text(),$cfg->{rmasks}{DECODE});
  dp_text_set_fname($o,$cfg->{files}{out}{nf_ref30text});
  dp_text_set_header($o,'ref30');
  dp_text_set_channels($o,1);
  dp_set_inl($o,$buffers->{v_nf_ref30},$cfg->{blens}{nf});
 };


 $o = addObj($olist,'phase_meas',dp_create_ph_compare(),$cfg->{rmasks}{DECODE});
 dp_ph_compare_set_ins($o,$buffers->{v_nf_ref30},$buffers->{v_nf_var30});
 dp_set_runlen($o,$cfg->{blens}{nf});


 $o = addObj($olist,'morse_decimator',dp_create_decimate(),$cfg->{rmasks}{DECODE});
 dp_decimate_set_decim($o,$cfg->{srates}{af} / $cfg->{srates}{mo});
 dp_set_inlt($o,$buffers->{v_af_agcd_audio},$cfg->{blens}{af},0);
 dp_set_out($o,$buffers->{v_mo_morse_audio_unf});


 $o = setupFilter($cfg,'bp1020_sr6400','bpf_id_tone');
 dp_set_inlt($o,$buffers->{v_mo_morse_audio_unf},$cfg->{blens}{mo},0);
 dp_set_out($o,$buffers->{v_mo_morse_audio});


 if ($cfg->{debug}) {
  $o = addObj($olist,'tone_writer',dp_create_wav_w(),$cfg->{rmasks}{DECODE});
  dp_wav_w_set_num_channels($o,1);
  dp_wav_w_set_bits_per_sample($o,16);
  dp_wav_w_set_sample_rate($o,$cfg->{srates}{mo});
  dp_wav_w_set_fname($o,$cfg->{files}{out}{mo_tone1020});
  dp_set_inl($o,$buffers->{v_mo_morse_audio},$cfg->{blens}{mo});
 };


 $o = addObj($olist,'morse_envelope',dp_create_envelope(),$cfg->{rmasks}{DECODE});
 dp_envelope_set_sample_rate($o,$cfg->{srates}{mo});
 dp_envelope_set_attack_s($o,0.03);
 dp_envelope_set_release_s($o,0.03);
 dp_set_inl($o,$buffers->{v_mo_morse_audio},$cfg->{blens}{mo});
 dp_set_out($o,$buffers->{v_mo_morse_env});


 if ($cfg->{debug}) {
  $o = addObj($olist,'morse_env_text',dp_create_text(),$cfg->{rmasks}{DECODE});
  dp_text_set_header($o,'morse_env');
  dp_text_set_channels($o,1);
  dp_text_set_fname($o, $cfg->{files}{out}{mo_morseenv});
  dp_set_inl($o,$buffers->{v_mo_morse_env},$cfg->{blens}{mo});
 }


 $o = addObj($olist,'morse_thresholder',dp_create_thresh(),$cfg->{rmasks}{DECODE});
 dp_thresh_set_autothresh($o,0);
 dp_thresh_set_thresholds($o,500, 800);
 dp_set_inl($o,$buffers->{v_mo_morse_env},$cfg->{blens}{mo});
 dp_set_out($o,$buffers->{v_mo_morse_binary});


 if ($cfg->{debug}) {
  $o = addObj($olist,'morse_binary_text',dp_create_text(),$cfg->{rmasks}{DECODE});
  dp_text_set_fname($o,$cfg->{files}{out}{mo_morsebin});
  dp_text_set_header($o,'morse_env_binary');
  dp_text_set_channels($o,1);
  dp_set_inl($o,$buffers->{v_mo_morse_binary},$cfg->{blens}{mo});
 }


 $o = addObj($olist,'ditdah_detect',dp_create_ditdah(),$cfg->{rmasks}{DECODE});
 dp_ditdah_set_sample_rate($o,$cfg->{srates}{mo});
 dp_set_inl($o,$buffers->{v_mo_morse_binary},$cfg->{blens}{mo});
 dp_set_out($o,$buffers->{v_af_ditdah});
 my $pmorse_len = dp_pl_alloc_uint32_t();
 dp_pl_sharelist_add_u32p($sl,'dsp_pmorse_len',$pmorse_len);
 dp_ditdah_set_autolenout($o,$pmorse_len);

 # note that this is being added to the object list but I don't want
 # it run automatically by the baselist routine. Instead, I will run
 # it manually in the perl callback routine because we do not know
 # in advance how many symbols for it to run over.
 $o = addObj($olist,'morse_decoder',dp_create_morse(),$cfg->{rmasks}{DECODE});
 dp_set_in($o,$buffers->{v_af_ditdah});
 dp_morse_set_autolen($o,$pmorse_len);
 my $pmorse_str = dp_pl_alloc_str(20);
 dp_pl_sharelist_add_cp($sl,'dsp_pmorse_str',$pmorse_str);

 my $dsp_lrstat = {
  have_carrier => 0,
  buffer_count => 0,
  nf_sr => $cfg->{srates}{nf},
  block_time_ms => 1000 * $cfg->{blens}{nf} / $cfg->{srates}{nf},
  mixer => $olist->{named}{mixer}{obj},
  radio => $cfg->{use_radio} ? $olist->{named}{radio0}{obj} : 0,
  phase_diff_lpf => 0,
  strength_ratio_lpf => 0, 
 };
 $cfg->{dsp_lrstat} = $dsp_lrstat;
 $cfg->{rstat_queue} = Thread::Queue->new();
 $cfg->{fstat_queue} = Thread::Queue->new();
 $cfg->{buffer_ct} = 0;

 return $olist;
};

sub perl_dsp_after_cb {
 my $cfg = shift;
 # print "running dsp_after_cb\n";

 #dp_vec_show_n($cfg->{buffers}{v_if0_c_fft},20000);

 my $rmask = $cfg->{slg}->get_u32('runmask');
 my $rstat = $cfg->{dsp_lrstat};

 $rstat->{run_fft}        = $rmask & $cfg->{rmasks}{FFT};
 $rstat->{run_decode}     = $rmask & $cfg->{rmasks}{DECODE};
 $rstat->{have_status}    = 1;
 if ($rmask & $cfg->{rmasks}{DECODE}) {
  $rstat->{freq}           = $cfg->{slg}->get_u32('curr_freq');
  $rstat->{use_mixer}      = $cfg->{slg}->get_u32('use_mixer');
  $rstat->{lo_freq}        = $cfg->{slg}->get_i32('mixer_lo_freq');
  my $power_ratio = $cfg->{slg}->get_fpf('dsp_ppower_ratio');
  $rstat->{strength_ratio} = $power_ratio;
  $rstat->{strength_ratio_lpf} = (1 - $cfg->{lpf_factor}) * $rstat->{strength_ratio_lpf} +
                                 (    $cfg->{lpf_factor}) * $power_ratio;
  if (!looks_like_number($rstat->{strength_ratio_lpf}) || ($rstat->{strength_ratio_lpf} > 10000)) {
   $rstat->{strength_ratio_lpf} = $power_ratio;	 
  }
  # not clear if I want to track carrier by this block only
  # or by the lpf'd version. I'll go with the LPF for now.
  #$rstat->{have_carrier} = $power_ratio > $cfg->{min_carrier_snr};
  $rstat->{have_carrier} = $rstat->{strength_ratio_lpf} > $cfg->{min_carrier_snr};
 }  else {
  $rstat->{have_carrier} = 0;
 } 

 if ($rstat->{have_carrier}) {
  my $ph_meas = $cfg->{olist}{named}{phase_meas}{obj};
  my $ph_diff = dp_ph_compare_getLastBlockPhase($ph_meas) / $cfg->{srates}{nf};
  my $rp = dp_ph_compare_getLastBlockPeriod($ph_meas,0);
  my $vp = dp_ph_compare_getLastBlockPeriod($ph_meas,1);
  $rstat->{ref30_period}   = $rp ? $cfg->{srates}{nf} / $rp : 0;
  $rstat->{var30_period}   = $vp ? $cfg->{srates}{nf} / $vp : 0;
  $rstat->{phase_diff}     = $ph_diff;
  my $ph_diff_lpf          = (1 - $cfg->{lpf_factor}) * $rstat->{phase_diff_lpf} +
                             (    $cfg->{lpf_factor}) * $ph_diff;
  $rstat->{phase_diff_lpf} = $ph_diff_lpf;
  #print("ph_diff $ph_diff ph_diff_lpf $ph_diff_lpf\n");

  my $radial     = ($ph_diff     * 360) + $cfg->{radial_calibration};
  my $radial_lpf = ($ph_diff_lpf * 360) + $cfg->{radial_calibration};
  $radial     += ($radial     < 0) ? 360 : ($radial     > 360) ? -360 : 0;
  $radial_lpf += ($radial_lpf < 0) ? 360 : ($radial_lpf > 360) ? -360 : 0;
  $rstat->{radial}     = $radial;
  $rstat->{radial_lpf} = $radial_lpf;

  my $modec = $cfg->{olist}{named}{morse_decoder}{obj};
  my $iter_decoded_str = '';
  my $pmorse_str = $cfg->{slg}->get_cp('dsp_pmorse_str');
  dp_morse_get_decoded($modec,$pmorse_str);
  $rstat->{decoded_str} = dplib::cp_to_svstr($pmorse_str);
 }

 if ($rmask & $cfg->{rmasks}{FFT}) {
  my $ppts     = $cfg->{slg}->get_vp('dsp_ppeak_pts');
  my $fstat    = dplib::read_peak_pts_t($cfg->{sl},$ppts);
  my $fs_q     = $cfg->{fstat_queue};
  $fs_q->enqueue($fstat);
  if ($fs_q->pending() > $cfg->{max_rs_queue}) {
   $fs_q->dequeue_nb();
  }
 }

 $rstat->{buffer_count} = $cfg->{buffer_count}++;


 my $rstat_cpy = rs_copy($rstat);
 my $rs_q = $cfg->{rstat_queue};
 $rs_q->enqueue($rstat_cpy);
 if ($rs_q->pending() > $cfg->{max_rs_queue}) {
  $rs_q->dequeue_nb();
 }

 my $done = $cfg->{slg}->get_u32('dsp_done');
 if ($cfg->{use_radio}) {
  $done |=  ($cfg->{buffer_count} > $cfg->{max_buffer_count});
 } else {
  $done |= (dp_wav_r_get_last_elems($cfg->{olist}{named}{reader}{obj}) == 0);
 }
 $cfg->{slg}->set_u32('dsp_done',$done);
}


sub rs_copy {
 my $i = shift;
 my $o = {};
 foreach my $k (keys %$i) {
  $o->{$k} = $i->{$k};
 }
 return $o;
};


sub setupFilter {
 my $cfg = shift;
 my $fn  = shift;
 my $on  = shift;
 my $msk = shift;
 if (!defined($msk)) { $msk = $cfg->{rmasks}{DECODE}; };

 my @coeffs   = map { $_ * 32768; } @{$filters->{$fn}{taps}};
 my $coeffs_c = dplib::perlToCArray_dp_int_t(\@coeffs);
 my $o = undef;

 $o = addObj($cfg->{olist}, 
	     $on, 
	     $cfg->{use_fft_fir} ? dp_create_fft_fir() : dp_create_fir(),
	     $msk);

 # store copies of these in the list in case I ever get
 # around to properly deleting these refs, especially 
 # the c array.
 $cfg->{olist}{named}{$on}{filt_coeffs} = \@coeffs;
 $cfg->{olist}{named}{$on}{filt_coeffs_c} = $coeffs_c;

 if ($cfg->{use_fft_fir}) {
  dp_fft_fir_set_filter($o, $coeffs_c, scalar @coeffs,0);
 } else {
  dp_fir_set_filter($o, $coeffs_c, scalar @coeffs,0);
 }
 return $o;
};




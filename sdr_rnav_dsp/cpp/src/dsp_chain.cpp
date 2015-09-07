
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <sstream>
#include <ctime>
#include <string.h>
#include <assert.h>

// These are all DSP blocks; subclassed from block_base_c
#include "rtl_help.h"
#include "wav_w.h"
#include "wav_r.h"
#include "fir.h"
#include "decimate.h"
#include "envelope.h"
#include "text_dump.h"
#include "ditdah.h"
#include "morsedec.h"
#include "cmix.h"
#include "c2r.h"
#include "dcblock.h"
#include "quasi_quad.h"
#include "ph_compare.h"
#include "ecompare.h"
#include "x_corr.h"
#include "linaudio.h"
#include "cfft.h"
#include "find_peaks.h"
#include "fft_filt.h"


#include "autofilter.h"
#include "dsp_chain.h"
#include "my_console.h"
#ifndef INLINE_PERL
#include "main.h"
#else
#include "top_level.h"
#endif
#include "receiver_stat.h"
#include "conc_str_queue.h"
#include "conc_rstat_queue.h"
#include "conc_peaks_queue.h"

using namespace djdsp;

concurrent_str_queue_c   _dsp_chain_id_text_queue;
concurrent_rstat_queue_c _dsp_chain_rstat_queue;
concurrent_peaks_queue_c _dsp_chain_peaks_queue;
bool                     _dsp_chain_all_done;

const uint32_t RUN_MASK_NEVER   = 0x0;
const uint32_t RUN_MASK_ALWAYS  = 0x1;
const uint32_t RUN_MASK_FFT     = 0x2;
const uint32_t RUN_MASK_MIXER   = 0x4;
const uint32_t RUN_MASK_DECODE  = 0x8;

const int rtl_buf_count          = 2;
const int max_rstat_queue_length = 10;
const uint32_t if_sr = 250000;
const uint32_t af_sr = 31250;
const uint32_t nf_sr = 1250;
const uint32_t mo_sr = 6250;

const uint32_t if_buffer_len = 2*3*4*1024;

const uint32_t subcarrier_freq    = 9960;
const uint32_t subcarrier_max_dev = 480;
const float    nav_freq           = 30.0;

uint32_t af_buffer_len = if_buffer_len / (if_sr/af_sr);
uint32_t nf_buffer_len = af_buffer_len / (af_sr/nf_sr);
uint32_t mo_buffer_len = af_buffer_len / (af_sr/mo_sr);

dvec_t read_buffer             (if_buffer_len*2);
dvec_t fft_buffer              (if_buffer_len*2);
dvec_t fft_mag_buffer          (if_buffer_len);
dvec_t mixed_buffer            (if_buffer_len*2);
dvec_t lpfd_bb_buffer          (if_buffer_len*2);
dvec_t decim_bb_buffer         (af_buffer_len*2);
dvec_t real_bb_buffer          (af_buffer_len);
dvec_t unfiltered_audio_buffer (af_buffer_len);
dvec_t nodc_audio_buffer       (af_buffer_len);
dvec_t var30_first_pass_buffer (af_buffer_len);
dvec_t var30_decim_buffer      (nf_buffer_len);
dvec_t var30_buffer            (nf_buffer_len);
dvec_t var100_buffer           (af_buffer_len);
dvec_t fmcarrier_buffer        (af_buffer_len);
dvec_t ref30_buffer_1          (af_buffer_len);
dvec_t ref30_buffer_decimated  (nf_buffer_len);
dvec_t ref30_buffer            (nf_buffer_len);
dvec_t morse_audio_buffer_unf  (mo_buffer_len);
dvec_t morse_audio_buffer_flt  (mo_buffer_len);
dvec_t morse_contour_buffer    (mo_buffer_len);
dvec_t morse_binary_buffer     (mo_buffer_len); 
dvec_t ditdah_buffer           (af_buffer_len);

uint32_t _dsp_chain_current_runmask = 
                                      RUN_MASK_ALWAYS | 
                                      RUN_MASK_MIXER | 
				      RUN_MASK_DECODE |
				      0;

// routine to toggle the mixer or FFT in or not as desired.
// This also disables most of the decode chain if the FFT
// is turned on.
void toggleMixer(bool use_mixer,
		 bool do_fft,
		 cmixer_c *pmixer, 
		 block_base_c *plp_baseband) {
  bool mix = use_mixer && 
	     !pmixer->set_lo_freq(_main_mixer_lo_freq);
  if (do_fft) {
   _dsp_chain_current_runmask |= RUN_MASK_FFT;
   _dsp_chain_current_runmask &= ~RUN_MASK_DECODE;
   _dsp_chain_current_runmask &= ~RUN_MASK_MIXER;
  } else if (mix) {
   _dsp_chain_current_runmask &= ~RUN_MASK_FFT;
   _dsp_chain_current_runmask |= RUN_MASK_DECODE;
   _dsp_chain_current_runmask |= RUN_MASK_MIXER;
   plp_baseband->set_in(&mixed_buffer);
  } else {
   _dsp_chain_current_runmask &= ~RUN_MASK_FFT;
   _dsp_chain_current_runmask &= ~RUN_MASK_MIXER;
   _dsp_chain_current_runmask |= RUN_MASK_DECODE;
   plp_baseband->set_in(&read_buffer);
  }
}

extern "C" {

void *dsp_thread_fn(void *f) {

 int last_uses = 0;
 _dsp_chain_all_done = false;
 blockps_t blockps_dec;

#ifndef USE_RADIO
 // setup and initialize input 
 wav_r_c reader;
 blockps_dec.push_back(&reader);
 reader.set_name("reader");
 reader.set_out(&read_buffer);
 reader.set_runlen(if_buffer_len);
 reader.set_file(_main_infile);
 reader.set_group(RUN_MASK_ALWAYS);
#else
 rtl_help_c *rhelp = (rtl_help_c *)f;
 rhelp->set_buffers(if_buffer_len,rtl_buf_count);
 rhelp->set_out(&read_buffer);
 rhelp->dev_cmd(ST_FREQ,_main_freq);
 rhelp->set_group(RUN_MASK_ALWAYS);
 uint32_t real_sr = rhelp->dev_cmd(ST_SR,if_sr);
 std::cout << "sample rate return: " << real_sr << std::endl;
 rhelp->dev_cmd(ST_AGC,1);
 rhelp->dev_cmd(ST_GAIN_MANUAL,0);
 blockps_dec.push_back(rhelp);
#endif

 ///////////////////////////
 // Optional FFT -- this is for "fast" signal search
 //
 cfft_c fft;
 fft.set_name("fft");
 fft.set_out(&fft_buffer);
 fft.set_averaging(0.9);
 fft.set_in(&read_buffer,if_buffer_len,s_complex);
 fft.set_group(RUN_MASK_FFT);
 blockps_dec.push_back(&fft);

 c2r_c c2r_fft;
 c2r_fft.set_name("fft_c2r");
 c2r_fft.set_runlen(if_buffer_len);
 c2r_fft.set_in(&fft_buffer,if_buffer_len,s_complex);
 c2r_fft.set_out(&fft_mag_buffer);
 c2r_fft.set_group(RUN_MASK_FFT);
 blockps_dec.push_back(&c2r_fft);
 
 findpeaks_c fftpks;
 fftpks.set_name("fft_peaks");
 fftpks.set_in(&fft_mag_buffer,if_buffer_len);
 fftpks.set_group(RUN_MASK_FFT);
 peak_pts_t      local_peaks;
 fftpks.set_out(&local_peaks);
 blockps_dec.push_back(&fftpks);
 //
 ///////////////////////////

#if 0
 text_dump_c fft_text;
 fft_text.set_name("fft_text");
 fft_text.set_file(odir + "fft.csv","real,im");
 fft_text.set_channels(2);
 fft_text.set_in(&fft_buffer);
 fft_text.set_runlen(if_buffer_len);
 fft_text.set_group(RUN_MASK_FFT);
 blockps_dec.push_back(&fft_text);
#endif

 // setup and initialize first mixer
 cmixer_c mixer;
 mixer.set_name("mixer");
 mixer.set_sample_rate(if_sr);
 mixer.set_in(&read_buffer,if_buffer_len);
 mixer.set_out(&mixed_buffer);
 mixer.set_lo_freq(_main_mixer_lo_freq);
 mixer.set_group(RUN_MASK_MIXER);
 blockps_dec.push_back(&mixer);


 // setup and initialize low pass for post-mixer
 /*
__firdes__start__yaml__
---
 - name: lp25k_sr250k
   type: fir
   subtype: bandpass
   sample_rate: 250000 
   bands:
    - start: 0
      end: 20k
    - start: 30k
      end: 125000
   mags:
    - 1
    - 0
   ripple:
    - 1dB
    - -70dB
__firdes__end__yaml__
*/
#ifdef FFT_FIR
 fft_filt_c lp_baseband;
#else
 fir_c lp_baseband;
#endif
 lp_baseband.set_name("lp_baseband");
 lp_baseband.set_filter(lp25k_sr250k_coeffs,lp25k_sr250k_len,s_real);
 lp_baseband.set_in(&mixed_buffer,if_buffer_len,s_complex);
 lp_baseband.set_group(RUN_MASK_DECODE);
 lp_baseband.set_out(&lpfd_bb_buffer);
 blockps_dec.push_back(&lp_baseband);

 toggleMixer(_main_use_mixer, _main_perform_fft, &mixer, &lp_baseband);
 
 // setup and initialize baseband decimator
 decimate_c bb_decimator;
 bb_decimator.set_name("bb_decimator");
 bb_decimator.set_decim(if_sr/af_sr);
 bb_decimator.set_in(&lpfd_bb_buffer,if_buffer_len,s_complex);
 bb_decimator.set_out(&decim_bb_buffer);
 bb_decimator.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&bb_decimator);

#ifdef DEBUG
 // write the simple baseband, before decimation
 wav_w_c bb_writer;
 bb_writer.set_name("bb_writer");
 bb_writer.set_num_channels(2);
 bb_writer.set_sample_rate(af_sr);
 bb_writer.set_bits_per_sample(16);
 bb_writer.set_in(&decim_bb_buffer,af_buffer_len);
 bb_writer.set_file(odir + "post_mix.wav");
 bb_writer.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&bb_writer);
#endif
 
 // convert complex to real signal with magnitudes
 c2r_c complex2mag;
 complex2mag.set_name("complex2mag");
 complex2mag.set_in(&decim_bb_buffer,af_buffer_len,s_complex);
 complex2mag.set_out(&real_bb_buffer);
 complex2mag.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&complex2mag);

 // am demodulate with envelope follower
 envelope_c am_env;
 am_env.set_name("am_env");
 am_env.set_sample_rate(af_sr);
 am_env.set_attack_s(0.0002);
 am_env.set_release_s(0.0002);
 am_env.set_in(&real_bb_buffer,af_buffer_len);
 am_env.set_out(&unfiltered_audio_buffer);
 am_env.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&am_env);

 // remove any dc bias
 dcblock_c dc_block;
 dc_block.set_name("dc_block");
 dc_block.set_pole(0.99999); // current implementation does not use
 dc_block.set_in(&unfiltered_audio_buffer,af_buffer_len);
 dc_block.set_out(&nodc_audio_buffer);
 dc_block.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&dc_block);

#ifdef DEBUG
 // write the audio file
 wav_w_c audio_writer;
 audio_writer.set_name("audio_writer");
 audio_writer.set_num_channels(1);
 audio_writer.set_sample_rate(af_sr);
 audio_writer.set_bits_per_sample(16);
 audio_writer.set_file( odir + "decoded_audio.wav");
 audio_writer.set_in(&nodc_audio_buffer,af_buffer_len);
 audio_writer.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&audio_writer);
#endif

#ifdef __linux
#ifdef AUDIO_OUTPUT
 linux_audio_c la;
 la.set_name("audio_output");
 la.set_num_channels(1);
 la.set_sample_rate(af_sr);
 la.set_bits_per_sample(16);
 la.set_in(&nodc_audio_buffer,af_buffer_len);
 la.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&la);
#endif
#endif


/*
__firdes__start__yaml__
---
 - name: lp400_sr31250
   type: fir
   subtype: bandpass
   sample_rate: 31250
   bands:
    - start: 0
      end: 400
    - start: 700
      end: 15625
   mags:
    - 1
    - 0
   ripple:
    - 1dB
    - -70dB
__firdes__end__yaml__
*/
 // first filter the am demodulated signal as low 
 // as we can get the filter designer to go
#ifdef FFT_FIR
 fft_filt_c lpf_var30_1;
#else
 fir_c lpf_var30_1;
#endif
 lpf_var30_1.set_name("lpf_var30_1");
 lpf_var30_1.set_filter(lp400_sr31250_coeffs,lp400_sr31250_len,s_real);
 lpf_var30_1.set_in(&nodc_audio_buffer,af_buffer_len,s_real);
 lpf_var30_1.set_out(&var30_first_pass_buffer);
 lpf_var30_1.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&lpf_var30_1);

 // decimate the Var 30 Hz signal to the "navigation"
 // sample rate
 decimate_c var30_decimator;
 var30_decimator.set_name("var30_decimator");
 uint32_t var30_decim = af_sr/nf_sr;
 d3(af_buffer_len,nf_buffer_len,var30_decim);
 var30_decimator.set_decim(var30_decim);
 var30_decimator.set_in(&var30_first_pass_buffer,af_buffer_len,s_real);
 var30_decimator.set_out(&var30_decim_buffer);
 var30_decimator.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&var30_decimator);

/*
__firdes__start__yaml__
---
 - name: bp30_sr1250
   type: fir
   subtype: bandpass
   sample_rate: 1250 
   num_taps: 250
   bands:
    - start: 0
      end: 20 
    - start: 25
      end: 35
    - start: 40
      end: 625
   mags:
    - 0
    - 1
    - 0
   ripple:
    - -60dB
    - 1dB
    - -60dB
__firdes__end__yaml__
*/
 // now, at the low sample rate, bandpass for 30 Hz.
#ifdef FFT_FIR
 fft_filt_c bpf_var30_2;
#else
 fir_c bpf_var30_2;
#endif
 bpf_var30_2.set_name("bpf_var30_2");
 bpf_var30_2.set_filter(bp30_sr1250_coeffs,bp30_sr1250_len,s_real);
 bpf_var30_2.set_in(&var30_decim_buffer,nf_buffer_len,s_real);
 bpf_var30_2.set_out(&var30_buffer);
 bpf_var30_2.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&bpf_var30_2);

#ifdef DEBUG
 text_dump_c var30_decim_text;
 var30_decim_text.set_name("var30_decim_text");
 var30_decim_text.set_file(odir + "var30_decim.csv","var30_decim");
 var30_decim_text.set_channels(1);
 var30_decim_text.set_in(&var30_decim_buffer,nf_buffer_len);
 var30_decim_text.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&var30_decim_text);
#endif

/*
__firdes__start__yaml__
---
 - name: bp105_sr1250
   type: fir
   subtype: bandpass
   num_taps: 250
   sample_rate: 1250 
   bands:
    - start: 0
      end: 95 
    - start: 100
      end: 110 
    - start: 115 
      end: 625
   mags:
    - 0
    - 1
    - 0
   ripple:
    - -60dB
    - 1dB
    - -60dB
__firdes__end__yaml__
*/
 
 /*

   Carrier detection scheme mark I:

   how do we even know we're receiving any station at all?  What I do
   here is after demodulating the AM signal, I then filter not only the
   30 Hz signal, but a 105 Hz signal -- that should not exist. (We then
   compare the power in the two filtered signals. The 30 Hz signal should
   have some, the 105 Hz signal should have not more than noise. If the
   ratio of power of the 30 Hz filtered signal to the 105Hz signal is
   over some threshold, then we are probably looking at a VOR signal --
   or at least a signal in the VOR range with a 30 Hz modulation and no
   100 Hz modulation. I suppose that a VOR might have interference at
   105Hz, in which case, this scheme will fail.

   105 Hz was chosen because it is a couple octaves away from 30 Hz and
   is dead in between the 3rd and 4th harmonics.

 */

#ifdef FFT_FIR
 fft_filt_c bpf_var100;
#else
 fir_c bpf_var100;
#endif 
 bpf_var100.set_name("bpf_var100");
 bpf_var100.set_filter(bp105_sr1250_coeffs,bp105_sr1250_len,s_real);
 bpf_var100.set_in(&var30_decim_buffer,nf_buffer_len,s_real);
 bpf_var100.set_out(&var100_buffer);
 bpf_var100.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&bpf_var100);

#ifdef DEBUG
 text_dump_c var100_text;
 var100_text.set_name("var100_text");
 var100_text.set_file(odir + "var100.csv","var100");
 var100_text.set_channels(1);
 var100_text.set_in(&var100_buffer,nf_buffer_len);
 var100_text.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&var100_text);
#endif

 ecompare_c ecmp1;
 float ratio = 0;
 ecmp1.set_name("ecompare_var30_var100");
 ecmp1.set_runlen(nf_buffer_len);
 ecmp1.set_in_1(&var30_buffer);
 ecmp1.set_in_2(&var100_buffer);
 ecmp1.set_out(&ratio);
 ecmp1.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&ecmp1);
 
#ifdef DEBUG
 // write a wav file for verification
 wav_w_c var30_writer;
 var30_writer.set_name("var30_writer");
 var30_writer.set_num_channels(1);
 var30_writer.set_sample_rate(nf_sr);
 var30_writer.set_bits_per_sample(16);
 var30_writer.set_in(&var30_buffer,nf_buffer_len);
 var30_writer.set_file(odir + "var30.wav");
 var30_writer.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&var30_writer);
#endif

/*
__firdes__start__yaml__
---
 - name: bp9960_sr31250
   num_taps: 275 
   type: fir
   subtype: bandpass
   sample_rate: 31250 
   bands:
    - start: 0
      end: 9300
    - start: 9450 
      end: 10470
    - start: 10620
      end: 15625
   mags:
    - 0
    - 1
    - 0
   ripple:
    - -60dB
    - 1dB
    - -60dB
__firdes__end__yaml__
*/
 
 // now work on the ref30 wave
 // 9960 bandpass
#ifdef FFT_FIR
 fft_filt_c bpf_fmcarrier;
#else
 fir_c bpf_fmcarrier;
#endif
 bpf_fmcarrier.set_name("bpf_fmcarrier");
 bpf_fmcarrier.set_filter(bp9960_sr31250_coeffs,bp9960_sr31250_len,s_real);
 bpf_fmcarrier.set_in(&real_bb_buffer,af_buffer_len,s_real);
 bpf_fmcarrier.set_out(&fmcarrier_buffer); 
 bpf_fmcarrier.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&bpf_fmcarrier);

#ifdef DEBUG
 // for debugging, write the 9960 carrier to a wave file
 wav_w_c c9960_writer;
 c9960_writer.set_name("c9960_writer");
 c9960_writer.set_num_channels(1);
 c9960_writer.set_sample_rate(af_sr);
 c9960_writer.set_bits_per_sample(16);
 c9960_writer.set_in(&fmcarrier_buffer,af_buffer_len);
 c9960_writer.set_file(odir + "c9960.wav");
 c9960_writer.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&c9960_writer);
#endif

 // This performs a frequeny to period conversion ... badly
 quasi_quad_c fm_demod;
 fm_demod.set_name("fm_demod");
 fm_demod.setup(af_sr,subcarrier_freq,subcarrier_max_dev);
 fm_demod.set_in(&fmcarrier_buffer,af_buffer_len);
 fm_demod.set_out(&ref30_buffer_1);
 fm_demod.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&fm_demod);


 // And we decimate down to a rate we can do low freq
 // filtering without too much pain
 uint32_t ref30_decim = af_sr/nf_sr;
 decimate_c ref30_decimator;
 ref30_decimator.set_name("ref30_decimator");
 ref30_decimator.set_decim(ref30_decim);
 ref30_decimator.set_in(&ref30_buffer_1,af_buffer_len,s_real);
 ref30_decimator.set_out(&ref30_buffer_decimated);
 ref30_decimator.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&ref30_decimator);


 // This fir is identical to the final one for the var30 signal
#ifdef FFT_FIR
 fft_filt_c bpf_ref30;
#else
 fir_c bpf_ref30;
#endif
 bpf_ref30.set_name("bpf_ref30");
 bpf_ref30.set_filter(bp30_sr1250_coeffs,bp30_sr1250_len,s_real);
 bpf_ref30.set_in(&ref30_buffer_decimated,nf_buffer_len,s_real);
 bpf_ref30.set_out(&ref30_buffer);
 bpf_ref30.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&bpf_ref30);

#ifdef DEBUG
 // for debugging, write the ref30 wave a wav file
 wav_w_c ref30_writer;
 ref30_writer.set_name("ref30_writer");
 ref30_writer.set_num_channels(1);
 ref30_writer.set_sample_rate(nf_sr);
 ref30_writer.set_bits_per_sample(16);
 ref30_writer.set_in(&ref30_buffer,nf_buffer_len);
 ref30_writer.set_file(odir + "ref30.wav");
 ref30_writer.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&ref30_writer);

 text_dump_c ref30_text;
 ref30_text.set_name("ref30_text");
 ref30_text.set_file(odir + "ref30.csv","ref30");
 ref30_text.set_channels(1);
 ref30_text.set_in(&ref30_buffer,nf_buffer_len);
 ref30_text.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&ref30_text);
#endif

 // block to measure phase difference; uses zero
 // crossing estimation
 ph_compare_c phase_meas;
 phase_meas.set_name("phase_meas");
 phase_meas.set_ins(&ref30_buffer,&var30_buffer);
 phase_meas.set_runlen(nf_buffer_len);
 phase_meas.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&phase_meas);

 // alternative phase compare
 dvec_t cross_correl_buff(nf_buffer_len);
 x_corr_c xc;
 xc.set_name("cross_correlate");
 xc.set_ins(&ref30_buffer,&var30_buffer);
 xc.set_runlen(nf_buffer_len);
 xc.set_delay_len(nf_buffer_len);
 xc.set_out(&cross_correl_buff);
 xc.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&xc);

 //// following blocks are for ID decode
 // first decimate to a sample rate higher enough to 
 // capture 1020 Hz audio tone, but not much higher to
 // make filtering hard
 decimate_c morse_decimator;
 morse_decimator.set_name("morse_decimator");
 morse_decimator.set_decim(af_sr/mo_sr);
 morse_decimator.set_in(&nodc_audio_buffer,af_buffer_len,s_real);
 morse_decimator.set_out(&morse_audio_buffer_unf);
 morse_decimator.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&morse_decimator);

/*
__firdes__start__yaml__
---
 - name: bp1020_sr6250_TEST
   type: fir
   subtype: bandpass
   num_taps: 170
   sample_rate: 6250
   bands:
    - start: 0
      end: 940 
    - start: 990 
      end: 1050 
    - start: 1100 
      end: 3125 
   mags:
    - 0
    - 1
    - 0
   ripple:
    - -60dB
    - 1dB
    - -60dB
__firdes__end__yaml__
*/

 // bandpass narrowly on that 1020 Hz tone
#ifdef FFT_FIR
 fft_filt_c bpf_id_tone;
#else
 fir_c bpf_id_tone;
#endif
 bpf_id_tone.set_name("bpf_id_tone");
 bpf_id_tone.set_filter(bp1020_sr6250_TEST_coeffs,bp1020_sr6250_TEST_len,s_real);
 bpf_id_tone.set_in(&morse_audio_buffer_unf,mo_buffer_len,s_real);
 bpf_id_tone.set_out(&morse_audio_buffer_flt);
 bpf_id_tone.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&bpf_id_tone);

#ifdef DEBUG
 wav_w_c tone_writer;
 tone_writer.set_name("tone_writer");
 tone_writer.set_num_channels(1);
 tone_writer.set_sample_rate(mo_sr);
 tone_writer.set_bits_per_sample(16);
 tone_writer.set_in(&morse_audio_buffer_flt,mo_buffer_len);
 tone_writer.set_file(odir + "tone1020.wav");
 tone_writer.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&tone_writer);
#endif

 // capture the envelope of that signal
 envelope_c morse_env;
 morse_env.set_name("morse_envelope");
 morse_env.set_sample_rate(mo_sr);
 morse_env.set_attack_s(0.03);
 morse_env.set_release_s(0.03);
 morse_env.set_in(&morse_audio_buffer_flt,mo_buffer_len);
 morse_env.set_out(&morse_contour_buffer);
 morse_env.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&morse_env);

#ifdef DEBUG
 text_dump_c morse_env_text;
 morse_env_text.set_name("morse_env_text");
 morse_env_text.set_file(odir + "morse_env.csv","morse_env");
 morse_env_text.set_channels(1);
 morse_env_text.set_in(&morse_contour_buffer,mo_buffer_len);
 morse_env_text.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&morse_env_text);
#endif

 // turn that envelope into a binary 1/0 proposition
 thresh_c morse_thresh;
 morse_thresh.set_name("morse_thresholder");
 // morse_thresh.set_thresholds(225,375);
 // morse_thresh.set_thresholds(40,80);
 morse_thresh.set_autothresh(true);
 morse_thresh.set_in(&morse_contour_buffer,mo_buffer_len);
 morse_thresh.set_out(&morse_binary_buffer);
 morse_thresh.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&morse_thresh);

#ifdef DEBUG
 text_dump_c morse_bin_text;
 morse_bin_text.set_name("morse_binary_text");
 morse_bin_text.set_file(odir + "morse_env_binary.csv","morse_env_binary");
 morse_bin_text.set_channels(1);
 morse_bin_text.set_in(&morse_binary_buffer,mo_buffer_len);
 morse_bin_text.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&morse_bin_text);
#endif

 // heuristics on timing to separate spaces from dits 
 // from dahs
 ditdah_c ditdah_detect;
 ditdah_detect.set_name("ditdah_detect");
 ditdah_detect.set_sample_rate(mo_sr);
 ditdah_detect.set_in(&morse_binary_buffer,mo_buffer_len);
 ditdah_detect.set_out(&ditdah_buffer);
 ditdah_detect.set_group(RUN_MASK_DECODE);
 blockps_dec.push_back(&ditdah_detect);

 morsedec_c morse_decoder;
 morse_decoder.set_name("morse_decoder");
 morse_decoder.set_in(&ditdah_buffer); 
 morse_decoder.set_group(RUN_MASK_DECODE);

 receiver_stat_t local_rstat;

 local_rstat.have_carrier = false;
 local_rstat.buffer_count = 0;
 local_rstat.nf_sr             = nf_sr;
 local_rstat.block_time_ms     = 1000.0 * (float)nf_buffer_len / (float)nf_sr;
 local_rstat.mixer             = &mixer;
#ifdef USE_RADIO
 local_rstat.radio             = rhelp;
#endif
 local_rstat.phase_diff_lpf = 0;

 std::cout << "-info- (dsp_chain) Performing pre-run setup..." << std::endl;
 for(uint32_t i=0;i<blockps_dec.size();i++) {
  blockps_dec.at(i)->pre_run();
 }
 std::cout << "-info- (dsp_chain) setup complete." << std::endl;
 // reader.file_info_str();
 time_t start = time(0);
 std::string fstr = "";
 uint32_t buffer_ct = 0;


 std::cout << "-info- (dsp_chain) Processing...." << std::endl;
 while (!_dsp_chain_all_done) { 

  if (1) {
   for(uint32_t i=0;i<blockps_dec.size();i++) {
    blockps_dec.at(i)->run(_dsp_chain_current_runmask);
   }

   if (_dsp_chain_current_runmask & RUN_MASK_DECODE) {
    local_rstat.tune_freq           = _main_freq;
    local_rstat.use_mixer           = _main_use_mixer;
    local_rstat.run_fft             = _main_perform_fft;
    local_rstat.mixer_lo_freq       = _main_mixer_lo_freq;
    local_rstat.strength_ratio      = ratio;
    local_rstat.strength_ratio_lpf  = (1.0 - PH_LPF_FACTOR) *
	                              local_rstat.strength_ratio_lpf +
				      PH_LPF_FACTOR * ratio;

    local_rstat.have_carrier      = (ratio > _main_min_snr);
    if (local_rstat.have_carrier) {
     float   period          = nf_sr / nav_freq;
     local_rstat.ref30_period      = phase_meas.getLastBlockPeriod(0);
     local_rstat.var30_period      = phase_meas.getLastBlockPeriod(1);
     float phase_diff              = phase_meas.getLastBlockPhase();
     if (phase_diff < 0) { 
      phase_diff += period; 
     };
     local_rstat.phase_diff        = phase_diff;
     float xcorr_delay             = xc.get_maxarg();
     local_rstat.xcorr_delay       = xcorr_delay;
     // create LPF'd versions of these values
     local_rstat.phase_diff_lpf    = (1.0-PH_LPF_FACTOR) * 
	                             local_rstat.phase_diff_lpf +
				     PH_LPF_FACTOR * phase_diff;
     local_rstat.xcorr_delay_lpf   = (1.0-PH_LPF_FACTOR) * 
	                             local_rstat.xcorr_delay_lpf +
				     PH_LPF_FACTOR * xcorr_delay;

     uint32_t dd_symbols = ditdah_detect.lastElems();
     if (dd_symbols) {
      morse_decoder.set_runlen(dd_symbols);
      morse_decoder.run(_dsp_chain_current_runmask);
      _dsp_chain_id_text_queue.push(morse_decoder.getDecoded());
     };
    };
    local_rstat.buffer_count      = buffer_ct;
    _dsp_chain_rstat_queue.push_max(local_rstat,max_rstat_queue_length);
   }

   if (_dsp_chain_current_runmask & RUN_MASK_FFT) {
    _dsp_chain_peaks_queue.push_max(local_peaks,max_rstat_queue_length);
   }
  }

#ifdef USE_RADIO
#ifndef INLINE_PERL
  if (_main_max_cap_seconds > 0) {
   _dsp_chain_all_done |= time(0) >= (start+_main_max_cap_seconds);
  }
#endif
#else
  _dsp_chain_all_done |= (reader.lastElems() == 0);
  if (_dsp_chain_all_done) {
   std::cout << "-info- (dsp_chain) wall clock elapsed: " << (time(0)-start) << "." << std::endl;
  }
#endif

  if (((_main_use_mixer<<1) | _main_perform_fft) != last_uses) {
   toggleMixer(_main_use_mixer,_main_perform_fft, &mixer,&lp_baseband);
  };

  last_uses = (_main_use_mixer << 1) | _main_perform_fft;
  // d3(_main_use_mixer, _main_perform_fft, _dsp_chain_current_runmask);

  buffer_ct++;
 }

 std::cout << "-info- (dsp_chain) Performing post-run cleanup..."
	   << std::endl;
 for(uint32_t i=0;i<blockps_dec.size();i++) {
  blockps_dec.at(i)->post_run();
 }
 std::cout << "-info (dsp_chain) post-run complete." << std::endl;

#ifdef USE_RADIO
 rhelp->stop_async(); 
#endif
 return NULL;
};

}; // extern "C"


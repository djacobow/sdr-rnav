
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "string_helper.h"

/* These are all DSP blocks; subclassed from block_base_c */
#include "dp_radio2832.h"
#include "dp_wav_w.h"
#include "dp_wav_r.h"
#include "dp_fir.h"
#include "dp_decimate.h"
#include "dp_envelope.h"
#include "dp_text.h"
#include "dp_ditdah.h"
#include "dp_thresh.h"
#include "dp_morse.h"
#include "dp_cmix.h"
#include "dp_c2r.h"
#include "dp_dcblock.h"
#include "dp_quasiquad.h"
#include "dp_ph_compare.h"
#include "dp_ecompare.h"
#include "dp_linaudio.h"
#include "dp_cfft.h"
#include "dp_findpeaks.h"
#include "dp_fft_fir.h"
#include "dp_agc.h"

#include "autofilter.h"
#include "dsp_chain.h"
#include "my_console.h"
#ifndef INLINE_PERL
#include "main.h"
#else
#include "top_level.h"
#endif
#include "receiver_stat.h"
#include "my_qs.h"

dp_conc_q_char_t  _dsp_chain_id_text_queue;
dp_conc_q_rstat_t _dsp_chain_rstat_queue;
dp_conc_q_peaks_t _dsp_chain_peaks_queue;
dp_bool_t         _dsp_chain_all_done;

uint32_t utemp;

const uint32_t RUN_MASK_NEVER   = 0x0;
const uint32_t RUN_MASK_ALWAYS  = 0x1;
const uint32_t RUN_MASK_FFT     = 0x2;
const uint32_t RUN_MASK_MIXER   = 0x4;
const uint32_t RUN_MASK_DECODE  = 0x8;

const int rtl_buf_count          = 2;
const int max_rstat_queue_length = 50;
const uint32_t if_sr = 250000;
const uint32_t af_sr = 31250;
const uint32_t nf_sr = 1250;
const uint32_t mo_sr = 6250;

const uint32_t if_buffer_len = 1*3*4*1024;

const uint32_t subcarrier_freq    = 9960;
const uint32_t subcarrier_max_dev = 480;
const float    nav_freq           = 30.0;

uint32_t _dsp_chain_current_runmask;

dp_vec_t *read_buffer, *mixed_buffer;

dp_baselist_t *bl;

/* routine to toggle the mixer or FFT in or not as desired.
   This also disables most of the decode chain if the FFT
   is turned on.
*/

void toggleMixer(dp_bool_t use_mixer,
		 dp_bool_t do_fft,
		 dp_base_t *pmixer, 
		 dp_base_t *plp_baseband) {
  dp_bool_t mix = use_mixer && 
	     !dp_cmix_set_lo_freq(pmixer,_main_mixer_lo_freq);
  if (do_fft) {
   fprintf(stderr,"-toggleMixer- fft scan ON\n");
   _dsp_chain_current_runmask |= RUN_MASK_FFT;
   _dsp_chain_current_runmask &= ~RUN_MASK_DECODE;
   _dsp_chain_current_runmask &= ~RUN_MASK_MIXER;
  } else if (mix) {
   fprintf(stderr,"-toggleMixer- mixer ON\n");
   _dsp_chain_current_runmask &= ~RUN_MASK_FFT;
   _dsp_chain_current_runmask |= RUN_MASK_DECODE;
   _dsp_chain_current_runmask |= RUN_MASK_MIXER;
   dp_set_in(plp_baseband,mixed_buffer);
  } else {
   fprintf(stderr,"-toggleMixer- mixer OFF\n");
   _dsp_chain_current_runmask &= ~RUN_MASK_FFT;
   _dsp_chain_current_runmask &= ~RUN_MASK_MIXER;
   _dsp_chain_current_runmask |= RUN_MASK_DECODE;
   dp_set_in(plp_baseband,read_buffer);
  }
}

void *dsp_thread_fn(void *f) {

 float power_ratio = 0;
 dp_base_t *radio;
 int last_uses = _main_use_mixer<<1 | _main_perform_fft;
 int curr_uses = last_uses;

 dp_vec_t *fft_buffer, 
	  *fft_mag_buffer, 
	  *lpfd_bb_buffer, 
	  *decim_bb_buffer,
          *real_bb_buffer, 
	  *unfiltered_audio_buffer, 
	  *nodc_audio_buffer, 
	  *agcd_audio_buffer, 
	  *var30_first_pass_buffer, 
	  *var30_decim_buffer, 
	  *var30_buffer, 
	  *var100_buffer, 
	  *fmcarrier_buffer, 
	  *ref30_buffer_1,
          *ref30_buffer_decimated, 
	  *ref30_buffer, 
	  *morse_audio_buffer_unf, 
	  *morse_audio_buffer_flt, 
	  *morse_contour_buffer, 
	  *morse_binary_buffer, 
	  *ditdah_buffer;

 dp_base_t *reader, 
	   *raw_writer,
	   *fft, 
	   *fft_text,
	   *c2r_fft, 
	   *fftpks, 
	   *mixer, 
	   *lp_baseband, 
	   *bb_decimator, 
	   *bb_writer, 
	   *complex2mag, 
	   *am_env, 
	   *dc_block, 
	   *audio_agc,
	   *audio_writer, 
	   *la, 
	   *lpf_var30_1, 
	   *var30_decimator, 
	   *bpf_var30_2, 
	   *var30_decim_text, 
	   *bpf_var100, 
	   *var100_text, 
	   *ecmp1, 
	   *var30_writer, 
	   *bpf_fmcarrier, 
	   *c9960_writer,
           *fm_demod, 
	   *ref30_decimator, 
	   *bpf_ref30, 
	   *ref30_writer, 
	   *ref30_text, 
	   *phase_meas, 
	   *morse_decimator, 
	   *bpf_id_tone, 
	   *tone_writer, 
	   *morse_env, 
	   *morse_env_text, 
	   *morse_thresh, 
	   *morse_bin_text, 
	   *ditdah_detect, 
	   *morse_decoder;

#ifdef USE_RADIO
 uint32_t  real_sr; 
#endif
 peak_pts_t      local_peaks;
 receiver_stat_t local_rstat;

 uint32_t var30_decim, ref30_decim;
 time_t start;
 uint32_t buffer_ct;
 char   iter_decoded_str[DP_MORSE_MAX_STR];
 char *temp_fname;


 const uint32_t af_buffer_len = if_buffer_len / (if_sr/af_sr);
 const uint32_t nf_buffer_len = af_buffer_len / (af_sr/nf_sr);
 const uint32_t mo_buffer_len = af_buffer_len / (af_sr/mo_sr);

 printf("af len %d\n",af_buffer_len);
 printf("nf len %d\n",nf_buffer_len);
 printf("mo len %d\n",mo_buffer_len);


 _dsp_chain_all_done = false;

 _dsp_chain_current_runmask = 
   RUN_MASK_ALWAYS | RUN_MASK_MIXER | RUN_MASK_DECODE | 0;


 memset(iter_decoded_str,0,DP_MORSE_MAX_STR);

 dp_conc_q_rstat_init(&_dsp_chain_rstat_queue,max_rstat_queue_length);
 dp_conc_q_peaks_init(&_dsp_chain_peaks_queue,max_rstat_queue_length);
 dp_conc_q_char_init(&_dsp_chain_id_text_queue,100);

 bl = dp_baselist_create(50);

 read_buffer             = dp_vec_create(if_buffer_len*2);
 fft_buffer              = dp_vec_create(if_buffer_len*2);
 fft_mag_buffer          = dp_vec_create(if_buffer_len);
 mixed_buffer            = dp_vec_create(if_buffer_len*2);
 lpfd_bb_buffer          = dp_vec_create(if_buffer_len*2);
 decim_bb_buffer         = dp_vec_create(af_buffer_len*2);
 real_bb_buffer          = dp_vec_create(af_buffer_len);
 unfiltered_audio_buffer = dp_vec_create(af_buffer_len);
 nodc_audio_buffer       = dp_vec_create(af_buffer_len);
 agcd_audio_buffer       = dp_vec_create(af_buffer_len);
 var30_first_pass_buffer = dp_vec_create(af_buffer_len);
 var30_decim_buffer      = dp_vec_create(nf_buffer_len);
 var30_buffer            = dp_vec_create(nf_buffer_len);
 var100_buffer           = dp_vec_create(af_buffer_len);
 fmcarrier_buffer        = dp_vec_create(af_buffer_len);
 ref30_buffer_1          = dp_vec_create(af_buffer_len);
 ref30_buffer_decimated  = dp_vec_create(nf_buffer_len);
 ref30_buffer            = dp_vec_create(nf_buffer_len);
 morse_audio_buffer_unf  = dp_vec_create(mo_buffer_len);
 morse_audio_buffer_flt  = dp_vec_create(mo_buffer_len);
 morse_contour_buffer    = dp_vec_create(mo_buffer_len);
 morse_binary_buffer     = dp_vec_create(mo_buffer_len); 
 ditdah_buffer           = dp_vec_create(af_buffer_len);


#ifndef USE_RADIO
 /* setup and initialize input  */
 DP_CREATE(reader,wav_r);
 dp_baselist_add(bl,reader);
 dp_set_name(reader,"reader");
 dp_set_out(reader,read_buffer);
 dp_set_runlen(reader,if_buffer_len);
 dp_wav_r_set_fname(reader,_main_infile);
 dp_set_group(reader,RUN_MASK_ALWAYS);
#else
 radio = (dp_base_t *)f;
 dp_radio2832_set_buffers(radio,if_buffer_len,rtl_buf_count);
 dp_set_out(radio,read_buffer);
 dp_radio2832_dev_cmd(radio,ST_FREQ,&_main_freq);
 dp_set_group(radio,RUN_MASK_ALWAYS);
 real_sr = if_sr;
 dp_radio2832_dev_cmd(radio, ST_SR,&real_sr);
 fprintf(stderr,"sample rate returned: %d\n",real_sr);

 /* auto gain */
 utemp = 0;
 dp_radio2832_dev_cmd(radio,ST_TUNER_GAIN_MODE,&utemp);

 /* agc on */
 utemp = 1;
 dp_radio2832_dev_cmd(radio,ST_AGC,&utemp);

 dp_baselist_add(bl,radio);
#endif

 if (0) {
  #if 1
 /* if you need to dump all the samples, things have 
  * REALLY gone wrong!
  */
  DP_CREATE(raw_writer,wav_w);
  dp_set_name(raw_writer,"raw_writer");
  dp_wav_w_set_num_channels(raw_writer,2);
  dp_wav_w_set_sample_rate(raw_writer,if_sr);
  dp_wav_w_set_bits_per_sample(raw_writer,16);
  dp_set_inl(raw_writer,read_buffer,if_buffer_len);
  temp_fname = create_cat_str(_main_odir, "raw_read.wav");
  dp_wav_w_set_fname(raw_writer,temp_fname);
  free(temp_fname);
  dp_set_group(raw_writer,RUN_MASK_ALWAYS);
  dp_baselist_add(bl,raw_writer);
  #else
  DP_CREATE(rbuffer_dumper,text);
  dp_set_name(rbuffer_dumper,"rbuffer_dumper");
  dp_text_set_header(rbuffer_dumper,"real,imag");
  temp_fname = create_cat_str(_main_odir, "rbuffer.csv");
  dp_text_set_fname(rbuffer_dumper, temp_fname);
  free(temp_fname);
  dp_text_set_channels(rbuffer_dumper,2);
  dp_set_inl(rbuffer_dumper,read_buffer,if_buffer_len);
  dp_set_group(rbuffer_dumper,RUN_MASK_ALWAYS);
  dp_baselist_add(bl,rbuffer_dumper);
  #endif
 }

 /*/////////////////////////
 // Optional FFT -- this is for "fast" signal search
 */
 DP_CREATE(fft,cfft);
 dp_set_name(fft,"fft");
 dp_set_out(fft,fft_buffer);
 dp_cfft_set_averaging(fft,0.8); 
 dp_set_inlt(fft,read_buffer,if_buffer_len,true);
 dp_set_group(fft,RUN_MASK_FFT);
 dp_baselist_add(bl,fft);

 DP_CREATE(c2r_fft,c2r);
 dp_set_name(c2r_fft,"fft_c2r");
 dp_set_runlen(c2r_fft,if_buffer_len);
 dp_set_inlt(c2r_fft,fft_buffer,if_buffer_len,true);
 dp_set_out(c2r_fft,fft_mag_buffer);
 dp_set_group(c2r_fft,RUN_MASK_FFT);
 dp_baselist_add(bl,c2r_fft);
 
#if 1
 DP_CREATE(fft_text,text);
 dp_set_name(fft_text,"fft_text");
 temp_fname = create_cat_str(_main_odir,"fft.csv");
 dp_text_set_fname(fft_text, temp_fname);
 free(temp_fname);
 dp_text_set_header(fft_text, "fft");
 dp_text_set_channels(fft_text,1);
 dp_set_inl(fft_text,fft_mag_buffer,if_buffer_len);
 dp_set_group(fft_text,RUN_MASK_FFT);
 dp_baselist_add(bl,fft_text);
#endif

 DP_CREATE(fftpks,findpeaks);
 dp_set_name(fftpks,"fft_peaks");
 dp_set_inl(fftpks,fft_mag_buffer,if_buffer_len);
 dp_set_group(fftpks,RUN_MASK_FFT);
 peak_pts_t_init(&local_peaks);
 dp_findpeaks_set_out(fftpks,&local_peaks);
 /* returns all peaks greater than pk_thresh x avg.
  * This is an arbitrary hack and should probably
  * be replaced with something based on science
  */
 dp_findpeaks_set_threshold(fftpks,_main_fft_pk_thresh);
 dp_baselist_add(bl,fftpks);

 /*
 ///////////////////////// */

 /* setup and initialize first mixer */
 DP_CREATE(mixer,cmix);
 dp_set_name(mixer,"mixer");
 dp_cmix_set_sample_rate(mixer,if_sr);
 dp_set_inlt(mixer,read_buffer,if_buffer_len,true);
 dp_set_out(mixer,mixed_buffer);
 dp_cmix_set_lo_freq(mixer,_main_mixer_lo_freq);
 dp_set_group(mixer,RUN_MASK_MIXER);
 dp_baselist_add(bl,mixer);

 /* setup and initialize low pass for post-mixer */
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
 DP_CREATE(lp_baseband,fft_fir);
 dp_fft_fir_set_filter(lp_baseband,lp25k_sr250k_coeffs,lp25k_sr250k_len,false);
#else
 DP_CREATE(lp_baseband,fir);
 dp_fir_set_filter(lp_baseband,lp25k_sr250k_coeffs,lp25k_sr250k_len,false);
#endif
 dp_set_name(lp_baseband,"lp_baseband");
 dp_set_inlt(lp_baseband,mixed_buffer,if_buffer_len,true);
 dp_set_group(lp_baseband,RUN_MASK_DECODE);
 dp_set_out(lp_baseband,lpfd_bb_buffer);
 dp_baselist_add(bl,lp_baseband);

 toggleMixer(_main_use_mixer, _main_perform_fft, mixer, lp_baseband);
 
 /* setup and initialize baseband decimator */
 DP_CREATE(bb_decimator,decimate);
 dp_set_name(bb_decimator,"bb_decimator");
 dp_decimate_set_decim(bb_decimator,if_sr/af_sr);
 dp_set_inlt(bb_decimator,lpfd_bb_buffer,if_buffer_len,true);
 dp_set_out(bb_decimator,decim_bb_buffer);
 dp_set_group(bb_decimator,RUN_MASK_DECODE);
 dp_baselist_add(bl,bb_decimator);

#ifdef DEBUG
 /* write the simple baseband, after decimation */
 DP_CREATE(bb_writer,wav_w);
 dp_set_name(bb_writer,"bb_writer");
 dp_wav_w_set_num_channels(bb_writer,2);
 dp_wav_w_set_sample_rate(bb_writer,af_sr);
 dp_wav_w_set_bits_per_sample(bb_writer,16);
 dp_set_inl(bb_writer,decim_bb_buffer,af_buffer_len);
 temp_fname = create_cat_str(_main_odir, "mixed_base.wav");
 dp_wav_w_set_fname(bb_writer,temp_fname);
 free(temp_fname);
 dp_set_group(bb_writer,RUN_MASK_DECODE);
 dp_baselist_add(bl,bb_writer);
#endif

 /* convert complex to real signal with magnitudes */
 DP_CREATE(complex2mag,c2r);
 dp_set_name(complex2mag,"complex2mag");
 dp_set_inlt(complex2mag,decim_bb_buffer,af_buffer_len,true);
 dp_set_out(complex2mag,real_bb_buffer);
 dp_set_group(complex2mag,RUN_MASK_DECODE);
 dp_baselist_add(bl,complex2mag);

 /* am demodulate with envelope follower */
 DP_CREATE(am_env,envelope);
 dp_set_name(am_env,"am_env");
 dp_envelope_set_sample_rate(am_env,af_sr);
 dp_envelope_set_attack_s(am_env,0.0002);
 dp_envelope_set_release_s(am_env,0.0002);
 dp_set_inl(am_env,real_bb_buffer,af_buffer_len);
 dp_set_out(am_env,unfiltered_audio_buffer);
 dp_set_group(am_env,RUN_MASK_DECODE);
 dp_baselist_add(bl,am_env);

 /* remove any dc bias */
 DP_CREATE(dc_block,dcblock);
 dp_set_name(dc_block,"dc_block");
 dp_dcblock_set_pole(dc_block,0.99999); /* current implementation does not use */
 dp_set_inl(dc_block,unfiltered_audio_buffer,af_buffer_len);
 dp_set_out(dc_block,nodc_audio_buffer);
 dp_set_group(dc_block,RUN_MASK_DECODE);
 dp_baselist_add(bl,dc_block);

 DP_CREATE(audio_agc,agc);
 dp_set_name(audio_agc,"audio_agc");
 dp_set_inl(audio_agc,nodc_audio_buffer,af_buffer_len);
 dp_set_out(audio_agc,agcd_audio_buffer);
 dp_set_group(audio_agc,RUN_MASK_DECODE);
 dp_agc_set_time_constant(audio_agc,0.8);
 dp_agc_set_desired_mag(audio_agc,40.0); 
 dp_baselist_add(bl,audio_agc);

#ifdef DEBUG
 /* write the audio file */
 DP_CREATE(audio_writer,wav_w);
 dp_set_name(audio_writer,"audio_writer");
 dp_wav_w_set_num_channels(audio_writer,1);
 dp_wav_w_set_sample_rate(audio_writer,af_sr);
 dp_wav_w_set_bits_per_sample(audio_writer,16);
 temp_fname = create_cat_str(_main_odir, "decoded_audio.wav");
 dp_wav_w_set_fname(audio_writer,temp_fname);
 free(temp_fname);
 dp_set_inl(audio_writer,agcd_audio_buffer,af_buffer_len);
 dp_set_group(audio_writer,RUN_MASK_DECODE);
 dp_baselist_add(bl,audio_writer);
#endif

#ifdef __linux
#ifdef AUDIO_OUTPUT
 DP_CREATE(la,linaudio);
 dp_set_name(la,"audio_output");
 dp_linaudio_set_num_channels(la,1);
 dp_linaudio_set_sample_rate(la,af_sr);
 dp_linaudio_set_bits_per_sample(la,16);
 dp_set_inl(la,agcd_audio_buffer,af_buffer_len);
 dp_set_group(la,RUN_MASK_DECODE);
 dp_baselist_add(bl,la);
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
 /* first filter the am demodulated signal as low as we can get the filter designer to go */
#ifdef FFT_FIR
 DP_CREATE(lpf_var30_1,fft_fir);
 dp_fft_fir_set_filter(lpf_var30_1,lp400_sr31250_coeffs,lp400_sr31250_len,false);
#else
 DP_CREATE(lpf_var30_1,fir);
 dp_fir_set_filter(lpf_var30_1,lp400_sr31250_coeffs,lp400_sr31250_len,false);
#endif
 dp_set_name(lpf_var30_1,"lpf_var30_1");
 dp_set_inlt(lpf_var30_1,agcd_audio_buffer,af_buffer_len,false);
 dp_set_out(lpf_var30_1,var30_first_pass_buffer);
 dp_set_group(lpf_var30_1,RUN_MASK_DECODE);
 dp_baselist_add(bl,lpf_var30_1);

 /* decimate the Var 30 Hz signal to the "navigation"  sample rate */
 DP_CREATE(var30_decimator,decimate);
 dp_set_name(var30_decimator,"var30_decimator");
 var30_decim = af_sr/nf_sr;
 dp_decimate_set_decim(var30_decimator,var30_decim);
 dp_set_inlt(var30_decimator,var30_first_pass_buffer,af_buffer_len,false);
 dp_set_out(var30_decimator,var30_decim_buffer);
 dp_set_group(var30_decimator,RUN_MASK_DECODE);
 dp_baselist_add(bl,var30_decimator);

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
 /* now, at the low sample rate, bandpass for 30 Hz. */
#ifdef FFT_FIR
 DP_CREATE(bpf_var30_2,fft_fir);
 dp_fft_fir_set_filter(bpf_var30_2,bp30_sr1250_coeffs,bp30_sr1250_len,false);
#else
 DP_CREATE(bpf_var30_2,fir);
 dp_fir_set_filter(bpf_var30_2,bp30_sr1250_coeffs,bp30_sr1250_len,false);
#endif
 dp_set_name(bpf_var30_2,"bpf_var30_2");
 dp_set_inlt(bpf_var30_2,var30_decim_buffer,nf_buffer_len,false);
 dp_set_out(bpf_var30_2,var30_buffer);
 dp_set_group(bpf_var30_2,RUN_MASK_DECODE);
 dp_baselist_add(bl,bpf_var30_2);

#ifdef DEBUG
 DP_CREATE(var30_decim_text,text);
 dp_set_name(var30_decim_text,"var30_text");
 dp_text_set_header(var30_decim_text,"var30");
 temp_fname = create_cat_str(_main_odir, "var30.csv");
 dp_text_set_fname(var30_decim_text, temp_fname);
 free(temp_fname);
 dp_text_set_channels(var30_decim_text,1);
 dp_set_inl(var30_decim_text,var30_buffer,nf_buffer_len);
 dp_set_group(var30_decim_text,RUN_MASK_DECODE);
 dp_baselist_add(bl,var30_decim_text);
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
 DP_CREATE(bpf_var100,fft_fir);
 dp_fft_fir_set_filter(bpf_var100, bp105_sr1250_coeffs,bp105_sr1250_len,false);
#else
 DP_CREATE(bpf_var100,fir);
 dp_fir_set_filter(bpf_var100, bp105_sr1250_coeffs,bp105_sr1250_len,false);
#endif 
 dp_set_name(bpf_var100, "bpf_var100");
 dp_set_inlt(bpf_var100, var30_decim_buffer,nf_buffer_len,false);
 dp_set_out(bpf_var100, var100_buffer);
 dp_set_group(bpf_var100,RUN_MASK_DECODE);
 dp_baselist_add(bl,bpf_var100);

#ifdef DEBUG
 DP_CREATE(var100_text,text);
 dp_set_name(var100_text,"var100_text");
 temp_fname = create_cat_str(_main_odir,"var100.csv");
 dp_text_set_fname(var100_text, temp_fname);
 free(temp_fname);
 dp_text_set_header(var100_text, "var100");
 dp_text_set_channels(var100_text,1);
 dp_set_inl(var100_text,var100_buffer,nf_buffer_len);
 dp_set_group(var100_text,RUN_MASK_DECODE);
 dp_baselist_add(bl,var100_text);
#endif

 DP_CREATE(ecmp1,ecompare);
 dp_set_name(ecmp1,"ecompare_var30_var100");
 dp_set_runlen(ecmp1,nf_buffer_len);
 dp_ecompare_set_ins(ecmp1,var30_buffer, var100_buffer);
 dp_ecompare_set_out(ecmp1,&power_ratio);
 dp_set_group(ecmp1,RUN_MASK_DECODE);
 dp_baselist_add(bl,ecmp1);
 
#ifdef DEBUG
 /* write a wav file for verification */
 DP_CREATE(var30_writer,wav_w);
 dp_set_name(var30_writer,"var30_writer");
 dp_wav_w_set_num_channels(var30_writer,1);
 dp_wav_w_set_sample_rate(var30_writer,nf_sr);
 dp_wav_w_set_bits_per_sample(var30_writer,16);
 dp_set_inl(var30_writer,var30_buffer,nf_buffer_len);
 temp_fname = create_cat_str(_main_odir, "var30.wav");
 dp_wav_w_set_fname(var30_writer, temp_fname);
 free(temp_fname);
 dp_set_group(var30_writer, RUN_MASK_DECODE);
 dp_baselist_add(bl,var30_writer);
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
 
 /* now work on the ref30 wave 9960 bandpass */
#ifdef FFT_FIR
 DP_CREATE(bpf_fmcarrier,fft_fir);
 dp_fft_fir_set_filter(bpf_fmcarrier,bp9960_sr31250_coeffs,bp9960_sr31250_len,false);
#else
 DP_CREATE(bpf_fmcarrier,fir);
 dp_fir_set_filter(bpf_fmcarrier,bp9960_sr31250_coeffs,bp9960_sr31250_len,false);
#endif
 dp_set_name(bpf_fmcarrier,"bpf_fmcarrier");
 dp_set_inlt(bpf_fmcarrier,real_bb_buffer,af_buffer_len,false);
 dp_set_out(bpf_fmcarrier,fmcarrier_buffer); 
 dp_set_group(bpf_fmcarrier,RUN_MASK_DECODE);
 dp_baselist_add(bl,bpf_fmcarrier);

#ifdef DEBUG
 /* for debugging, write the 9960 carrier to a wave file */
 DP_CREATE(c9960_writer,wav_w);
 dp_set_name(c9960_writer,"c9960_writer");
 dp_wav_w_set_num_channels(c9960_writer,1);
 dp_wav_w_set_sample_rate(c9960_writer,af_sr);
 dp_wav_w_set_bits_per_sample(c9960_writer,16);
 dp_set_inl(c9960_writer,fmcarrier_buffer,af_buffer_len);
 temp_fname = create_cat_str(_main_odir,"var100.csv");
 dp_wav_w_set_fname(c9960_writer, temp_fname);
 free(temp_fname);
 dp_set_group(c9960_writer,RUN_MASK_DECODE);
 dp_baselist_add(bl,c9960_writer);
#endif

 /* This performs a frequeny to period conversion ... badly */
 DP_CREATE(fm_demod,quasiquad);
 dp_set_name(fm_demod,"fm_demod");
 dp_quasiquad_setup(fm_demod,af_sr,subcarrier_freq,subcarrier_max_dev);
 dp_set_inl(fm_demod,fmcarrier_buffer,af_buffer_len);
 dp_set_out(fm_demod,ref30_buffer_1);
 dp_set_group(fm_demod,RUN_MASK_DECODE);
 dp_baselist_add(bl,fm_demod);


 /* And we decimate down to a rate we can do low freq
 filtering without too much pain */
 DP_CREATE(ref30_decimator,decimate);
 ref30_decim = af_sr/nf_sr;
 dp_set_name(ref30_decimator,"ref30_decimator");
 dp_decimate_set_decim(ref30_decimator,ref30_decim);
 dp_set_inlt(ref30_decimator,ref30_buffer_1,af_buffer_len,false);
 dp_set_out(ref30_decimator,ref30_buffer_decimated);
 dp_set_group(ref30_decimator,RUN_MASK_DECODE);
 dp_baselist_add(bl,ref30_decimator);


 /* This fir is identical to the final one for the var30 signal */
#ifdef FFT_FIR
 DP_CREATE(bpf_ref30,fft_fir);
 dp_fft_fir_set_filter(bpf_ref30,bp30_sr1250_coeffs,bp30_sr1250_len,false);
#else
 DP_CREATE(bpf_ref30,fir);
 dp_fir_set_filter(bpf_ref30,bp30_sr1250_coeffs,bp30_sr1250_len,false);
#endif
 dp_set_name(bpf_ref30,"bpf_ref30");
 dp_set_inlt(bpf_ref30,ref30_buffer_decimated,nf_buffer_len,false);
 dp_set_out(bpf_ref30,ref30_buffer);
 dp_set_group(bpf_ref30,RUN_MASK_DECODE);
 dp_baselist_add(bl,bpf_ref30);

#ifdef DEBUG
 /* for debugging, write the ref30 wave a wav file */
 DP_CREATE(ref30_writer,wav_w);
 dp_set_name(ref30_writer,"ref30_writer");
 dp_wav_w_set_num_channels(ref30_writer,1);
 dp_wav_w_set_sample_rate(ref30_writer,nf_sr);
 dp_wav_w_set_bits_per_sample(ref30_writer,16);
 dp_set_inl(ref30_writer,ref30_buffer,nf_buffer_len);
 temp_fname = create_cat_str(_main_odir, "ref30.wav");
 dp_wav_w_set_fname(ref30_writer,temp_fname);
 free(temp_fname);
 dp_set_group(ref30_writer,RUN_MASK_DECODE);
 dp_baselist_add(bl,ref30_writer);

 DP_CREATE(ref30_text,text);
 dp_set_name(ref30_text,"ref30_text");
 temp_fname = create_cat_str(_main_odir, "ref30.csv");
 dp_text_set_fname(ref30_text,temp_fname);
 dp_text_set_header(ref30_text,"ref30");
 free(temp_fname);
 dp_text_set_channels(ref30_text,1);
 dp_set_inl(ref30_text,ref30_buffer,nf_buffer_len);
 dp_set_group(ref30_text,RUN_MASK_DECODE);
 dp_baselist_add(bl,ref30_text);
#endif

 /* block to measure phase difference; uses zero
 // crossing estimation */
 DP_CREATE(phase_meas,ph_compare);
 dp_set_name(phase_meas,"phase_meas");
 dp_ph_compare_set_ins(phase_meas,ref30_buffer,var30_buffer);
 dp_set_runlen(phase_meas,nf_buffer_len);
 dp_set_group(phase_meas,RUN_MASK_DECODE);
 dp_baselist_add(bl,phase_meas);

 /*// following blocks are for ID decode
 first decimate to a sample rate higher enough to 
 capture 1020 Hz audio tone, but not much higher to
 make filtering hard */
 DP_CREATE(morse_decimator,decimate);
 dp_set_name(morse_decimator,"morse_decimator");
 dp_decimate_set_decim(morse_decimator,af_sr/mo_sr);
 dp_set_inlt(morse_decimator,agcd_audio_buffer,af_buffer_len,false);
 dp_set_out(morse_decimator,morse_audio_buffer_unf);
 dp_set_group(morse_decimator,RUN_MASK_DECODE);
 dp_baselist_add(bl,morse_decimator);

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

 /* bandpass narrowly on that 1020 Hz tone */
#ifdef FFT_FIR
 DP_CREATE(bpf_id_tone,fft_fir);
 dp_fft_fir_set_filter(bpf_id_tone,bp1020_sr6250_TEST_coeffs,bp1020_sr6250_TEST_len,false);
#else
 DP_CREATE(bpf_id_tone,fir);
 dp_fir_set_filter(bpf_id_tone,bp1020_sr6250_TEST_coeffs,bp1020_sr6250_TEST_len,false);
#endif
 dp_set_name(bpf_id_tone,"bpf_id_tone");
 dp_set_inlt(bpf_id_tone,morse_audio_buffer_unf,mo_buffer_len,false);
 dp_set_out(bpf_id_tone,morse_audio_buffer_flt);
 dp_set_group(bpf_id_tone,RUN_MASK_DECODE);
 dp_baselist_add(bl,bpf_id_tone);

#ifdef DEBUG
 DP_CREATE(tone_writer,wav_w);
 dp_set_name(tone_writer,"tone_writer");
 dp_wav_w_set_num_channels(tone_writer,1);
 dp_wav_w_set_sample_rate(tone_writer,mo_sr);
 dp_wav_w_set_bits_per_sample(tone_writer,16);
 dp_set_inl(tone_writer,morse_audio_buffer_flt,mo_buffer_len);
 temp_fname = create_cat_str(_main_odir, "tone1020.wav");
 dp_wav_w_set_fname(tone_writer,temp_fname);
 free(temp_fname);
 dp_set_group(tone_writer,RUN_MASK_DECODE);
 dp_baselist_add(bl,tone_writer);
#endif

 /* capture the envelope of that signal */
 DP_CREATE(morse_env,envelope);
 dp_set_name(morse_env,"morse_envelope");
 dp_envelope_set_sample_rate(morse_env,mo_sr);
 dp_envelope_set_attack_s(morse_env,0.03);
 dp_envelope_set_release_s(morse_env,0.03);
 dp_set_inl(morse_env,morse_audio_buffer_flt,mo_buffer_len);
 dp_set_out(morse_env,morse_contour_buffer);
 dp_set_group(morse_env,RUN_MASK_DECODE);
 dp_baselist_add(bl,morse_env);

#ifdef DEBUG
 DP_CREATE(morse_env_text,text);
 dp_set_name(morse_env_text,"morse_env_text");
 dp_text_set_header(morse_env_text,"morse_env");
 temp_fname = create_cat_str(_main_odir, "morse_env.csv");
 dp_text_set_fname(morse_env_text,temp_fname);
 free(temp_fname);
 dp_text_set_channels(morse_env_text,1);
 dp_set_inl(morse_env_text,morse_contour_buffer,mo_buffer_len);
 dp_set_group(morse_env_text,RUN_MASK_DECODE);
 dp_baselist_add(bl,morse_env_text);
#endif

 /* turn that envelope into a binary 1/0 proposition */
 DP_CREATE(morse_thresh,thresh);
 dp_set_name(morse_thresh,"morse_thresholder");
 dp_thresh_set_autothresh(morse_thresh,true);
 dp_set_inl(morse_thresh,morse_contour_buffer,mo_buffer_len);
 dp_set_out(morse_thresh,morse_binary_buffer);
 dp_set_group(morse_thresh,RUN_MASK_DECODE);
 dp_baselist_add(bl,morse_thresh);

#ifdef DEBUG
 DP_CREATE(morse_bin_text,text);
 dp_set_name(morse_bin_text,"morse_binary_text");
 temp_fname = create_cat_str(_main_odir, "morse_env_binary.csv");
 dp_text_set_fname(morse_bin_text,temp_fname);
 dp_text_set_header(morse_bin_text,"morse_env_binary");
 free(temp_fname);
 dp_text_set_channels(morse_bin_text,1);
 dp_set_inl(morse_bin_text,morse_binary_buffer,mo_buffer_len);
 dp_set_group(morse_bin_text,RUN_MASK_DECODE);
 dp_baselist_add(bl,morse_bin_text);
#endif

 /* heuristics on timing to separate spaces from dits / dahs */
 DP_CREATE(ditdah_detect,ditdah);
 dp_set_name(ditdah_detect,"ditdah_detect");
 dp_ditdah_set_sample_rate(ditdah_detect,mo_sr);
 dp_set_inl(ditdah_detect,morse_binary_buffer,mo_buffer_len);
 dp_set_out(ditdah_detect,ditdah_buffer);
 dp_set_group(ditdah_detect,RUN_MASK_DECODE);
 dp_baselist_add(bl,ditdah_detect);

 DP_CREATE(morse_decoder,morse);
 dp_set_name(morse_decoder,"morse_decoder");
 dp_set_in(morse_decoder,ditdah_buffer); 
 dp_set_group(morse_decoder,RUN_MASK_DECODE);

 local_rstat.have_carrier = false;
 local_rstat.buffer_count = 0;
 local_rstat.nf_sr             = nf_sr;
 local_rstat.block_time_ms     = 1000.0 * (float)nf_buffer_len / (float)nf_sr;
 local_rstat.mixer             = mixer;
#ifdef USE_RADIO
 local_rstat.radio             = radio;
#endif
 local_rstat.phase_diff_lpf = 0;
 local_rstat.strength_ratio_lpf = 0;
 fprintf(stderr,"-info- (dsp_chain) Performing pre-run setup...");
 dp_baselist_prerun(bl);
 fprintf(stderr,"...done\n");

 start = time(0);
 buffer_ct = 0;

 fprintf(stderr,"-info- (dsp_chain) Processing....\n");

 while (!_dsp_chain_all_done) { 
  uint32_t dd_symbols;

  if (1) {
   dp_baselist_run(bl,_dsp_chain_current_runmask);

   /* dp_vec_show_n(fft_mag_buffer, 15);   */

   if (_dsp_chain_current_runmask & RUN_MASK_DECODE) {
    local_rstat.tune_freq           = _main_freq;
    local_rstat.use_mixer           = _main_use_mixer;
    local_rstat.run_fft             = _main_perform_fft;
    local_rstat.mixer_lo_freq       = _main_mixer_lo_freq;
    local_rstat.strength_ratio      = power_ratio;
    local_rstat.strength_ratio_lpf  = (1.0 - PH_LPF_FACTOR) *
	                              local_rstat.strength_ratio_lpf +
				      PH_LPF_FACTOR * power_ratio;
    if ((local_rstat.strength_ratio_lpf != local_rstat.strength_ratio_lpf) ||
        (local_rstat.strength_ratio_lpf > 10000)) {
     local_rstat.strength_ratio_lpf = power_ratio;
    }
    local_rstat.have_carrier = (power_ratio > _main_min_snr);
    if (local_rstat.have_carrier) {
     float phase_diff              = dp_ph_compare_getLastBlockPhase(phase_meas) / (float)nf_sr;
     local_rstat.ref30_period      = (float)nf_sr / dp_ph_compare_getLastBlockPeriod(phase_meas,0);
     local_rstat.var30_period      = (float)nf_sr / dp_ph_compare_getLastBlockPeriod(phase_meas,1);
     local_rstat.phase_diff_lpf    = (1.0-PH_LPF_FACTOR) *
	                             local_rstat.phase_diff_lpf +
				     PH_LPF_FACTOR * phase_diff;
     local_rstat.phase_diff        = phase_diff;
    
     dd_symbols = dp_ditdah_get_last_elems(ditdah_detect);
     if (dd_symbols) {
      uint32_t i;
      dp_set_runlen(morse_decoder,dd_symbols);
      dp_run(morse_decoder,_dsp_chain_current_runmask);
      dp_morse_get_decoded(morse_decoder, iter_decoded_str);
      for (i=0;i<strlen(iter_decoded_str);i++) {
       char c = iter_decoded_str[i];
       dp_conc_q_char_push_max(&_dsp_chain_id_text_queue, &c);
      }
     };
    };
    local_rstat.buffer_count      = buffer_ct;
    dp_conc_q_rstat_push_max(&_dsp_chain_rstat_queue,&local_rstat);
    /* these interlock the control and decode threads so that both 
     * initialize by neither continues until the other is initialized.
     * There's probably a better way to do this.
     */
   }

   if (_dsp_chain_current_runmask & RUN_MASK_FFT) {
    dp_conc_q_peaks_push_max(&_dsp_chain_peaks_queue,&local_peaks);
   }

   _main_decode_has_begun |= 1;
   while (!_main_ctrl_has_begun) { };
  }

#ifdef USE_RADIO
  if (_main_max_cap_seconds > 0) {
   _dsp_chain_all_done |= time(0) >= (start+_main_max_cap_seconds);
  }
#else
  _dsp_chain_all_done |= (dp_wav_r_get_last_elems(reader) == 0);
  if (_dsp_chain_all_done) {
   fprintf(stderr, "-info- (dsp_chain) wall clock elapsed: %d.\n",(int)(time(0)-start));
  }
#endif

  curr_uses = (_main_use_mixer<<1) | _main_perform_fft;
  if (curr_uses != last_uses) {
   toggleMixer(_main_use_mixer,_main_perform_fft, mixer,lp_baseband);
  };

  last_uses = (_main_use_mixer << 1) | _main_perform_fft;

  buffer_ct++;
 }

 fprintf(stderr,"-info- (dsp_chain) Performing post-run cleanup...\n");
 dp_baselist_postrun(bl);
 fprintf(stderr,"-info- (dsp_chain) post-run complete.\n");

#ifdef USE_RADIO
 dp_radio2832_stop_async(radio);
#else
 dp_destroy(reader);
#endif

 dp_destroy(fft);
 dp_destroy(c2r_fft);
 dp_destroy(fft_text);
 dp_destroy(fftpks);
 dp_destroy(mixer);
 dp_destroy(lp_baseband);
 dp_destroy(bb_decimator);
 dp_destroy(bb_writer);
 dp_destroy(complex2mag);
 dp_destroy(am_env);
 dp_destroy(dc_block);
 dp_destroy(audio_agc);
 dp_destroy(audio_writer);
 dp_destroy(la);
 dp_destroy(lpf_var30_1);
 dp_destroy(var30_decimator);
 dp_destroy(bpf_var30_2);
 dp_destroy(var30_decim_text);
 dp_destroy(bpf_var100);
 dp_destroy(var100_text);
 dp_destroy(ecmp1);
 dp_destroy(var30_writer);
 dp_destroy(bpf_fmcarrier);
 dp_destroy(c9960_writer);
 dp_destroy(fm_demod);
 dp_destroy(ref30_decimator);
 dp_destroy(bpf_ref30);
 dp_destroy(ref30_writer);
 dp_destroy(ref30_text);
 dp_destroy(phase_meas);
 dp_destroy(morse_decimator);
 dp_destroy(bpf_id_tone);
 dp_destroy(tone_writer);
 dp_destroy(morse_env);
 dp_destroy(morse_env_text);
 dp_destroy(morse_thresh);
 dp_destroy(morse_bin_text);
 dp_destroy(ditdah_detect);
 dp_destroy(morse_decoder);

 dp_vec_destroy(fft_buffer); 
 dp_vec_destroy(fft_mag_buffer); 
 dp_vec_destroy(lpfd_bb_buffer); 
 dp_vec_destroy(decim_bb_buffer);
 dp_vec_destroy(real_bb_buffer); 
 dp_vec_destroy(unfiltered_audio_buffer);
 dp_vec_destroy(nodc_audio_buffer);
 dp_vec_destroy(agcd_audio_buffer);
 dp_vec_destroy(var30_first_pass_buffer);
 dp_vec_destroy(var30_decim_buffer); 
 dp_vec_destroy(var30_buffer); 
 dp_vec_destroy(var100_buffer);
 dp_vec_destroy(fmcarrier_buffer); 
 dp_vec_destroy(ref30_buffer_1);
 dp_vec_destroy(ref30_buffer_decimated); 
 dp_vec_destroy(ref30_buffer);
 dp_vec_destroy(morse_audio_buffer_unf);
 dp_vec_destroy(morse_audio_buffer_flt);
 dp_vec_destroy(morse_contour_buffer);
 dp_vec_destroy(morse_binary_buffer);
 dp_vec_destroy(ditdah_buffer);

 dp_baselist_destroy(bl);
 return NULL;
}


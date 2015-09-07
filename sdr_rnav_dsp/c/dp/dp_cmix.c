
/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "dp_cmix.h"

/*
// Implements a complex mixer class. Takes an IQ signal and performs
// a complex multiply with a sin/cos complex signal generated to match
// a given LO frequency provided. The LO frequency can be positive or 
// negative to slide the spectrum up or down. To slide up, provide a 
// negative value. 
*/

DP_SUB_CREATOR_IMPL(cmix)

DP_FN_PREAMBLE(cmix,init) {
 s->ready = 0;
 s->sine_len = 0;
 s->sine_allocd = 0;
 s->sin_wave = 0;
 s->cos_wave = 0;
 s->cos_start = 0;
 s->curr_sine_idx = 0;
 b->sub_work   = &dp_cmix_work;
 b->sub_deinit = &dp_cmix_deinit;
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(cmix,deinit) {
 if (s->sine_allocd) {
  free(s->sin_wave);
  free(s->cos_wave);
 }
}
DP_FN_POSTAMBLE

dp_bool_t 
dp_cmix_set_lo_freq(dp_base_t *b, float lf) {
 float abs_lo_freq;
 float sine_len_exact;
 float sine_repetitions_exact;
 uint32_t sine_repetitions;
 dp_cmix_sub_t *s = b->sub;
 if (DP_SUBVALID(b,cmix)) {
  s->lo_freq = lf;

  abs_lo_freq            = fabs(s->lo_freq);
  sine_len_exact         = (float)s->sample_rate / abs_lo_freq;
  sine_repetitions_exact = (float)SINE_ALLOC / sine_len_exact;
  sine_repetitions    = floor(sine_repetitions_exact);

  s->ready = 0;
  return (sine_repetitions == 0);
 }
 return 1;
}

void
dp_cmix_set_lo_amp(dp_base_t *b, float la) {
 dp_cmix_sub_t *s = b->sub;
 if (DP_SUBVALID(b,cmix)) {
  s->lo_amp = la;
  s->ready = 0;
 }
}

void
dp_cmix_set_sample_rate(dp_base_t *b, uint32_t sr) {
 dp_cmix_sub_t *s = b->sub;
 if (DP_SUBVALID(b,cmix)) {
  s->sample_rate = sr;
  s->ready = 0;
 }
}


uint32_t
dp_cmix_make_sine(dp_base_t *b) {
 dp_cmix_sub_t *s = b->sub;
 if (DP_SUBVALID(b,cmix)) {
  uint32_t i;
  float abs_lo_freq            = fabs(s->lo_freq);
  float sine_len_exact         = (float)s->sample_rate / abs_lo_freq;
  float sine_repetitions_exact = (float)SINE_ALLOC / sine_len_exact;
  uint32_t sine_repetitions    = floor(sine_repetitions_exact);

  if (!sine_repetitions) {
   fprintf(stderr,"-ce- cmix (%s). not enough samples in SINE_ALLOC to contain one full wave.\n",b->name);
   exit(-1);
  } else {
   fprintf(stderr,"-ci- cmixc (%s) created LO waveform f=%f\n" ,b->name,s->lo_freq);
  }
  s->sine_len    = floor(sine_len_exact*(float)sine_repetitions+0.5);

  if (!s->sine_allocd) {
   uint32_t alloc_size = SINE_ALLOC;
   s->sin_wave = malloc(sizeof(int16_t) * alloc_size);
   s->cos_wave = malloc(sizeof(int16_t) * alloc_size);
   s->sine_allocd = alloc_size;
  }

  for (i=0;i<s->sine_len;i++) {
   float angle = 2.0*PI *
	         (float)i *
		 (float)abs_lo_freq /
		 (float)s->sample_rate;

   float   s_val_f = sin(angle);
   int16_t s_val_i = 32767 * s_val_f;
   float   c_val_f = cos(angle);
   int16_t c_val_i = 32767 * c_val_f;
   s->sin_wave[i] = s_val_i;
   s->cos_wave[i] = c_val_i;
  }

  s->curr_sine_idx = 0;
  s->ready = 1;
  return s->sine_len;
 }
 return 0;
}


DP_FN_PREAMBLE(cmix,work) {
  uint32_t len = b->runlength;	
  uint32_t i;
  dp_bool_t reverse;
  if (!s->ready) {
   dp_cmix_make_sine(b);
  }

  reverse = s->lo_freq < 0;

  for (i=0;i<len;i++) {
   int16_t in_i = b->in_v->v[2*i] ;
   int16_t in_q = b->in_v->v[2*i+1];
   uint32_t idx = s->curr_sine_idx++ % s->sine_len;

   int16_t lo_i = reverse ? s->sin_wave[idx] : s->cos_wave[idx];
   int16_t lo_q = reverse ? s->cos_wave[idx] : s->sin_wave[idx];

   int32_t res_i_32 = (in_q * lo_i) + (in_i * lo_q);
   int32_t res_q_32 = (in_q * lo_q) - (in_i * lo_i);
   int16_t res_q_16 = SATURATE30(res_q_32) >> 15;
   int16_t res_i_16 = SATURATE30(res_i_32) >> 15;
  
   b->out_v->v[2*i]   = res_i_16;
   b->out_v->v[2*i+1] = res_q_16;
  }
}
DP_FN_POSTAMBLE



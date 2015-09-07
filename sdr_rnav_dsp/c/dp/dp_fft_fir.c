
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz
*/

#include "dp_fft_fir.h"
#include "kiss_fft.h"
#include "kiss_fftr.h"

/*

 This module is designed to replace the direct convolution 
 routines in fir_c with fft-based convolution routines. For
 the number of taps that most of my filters have, this 
 could be a BIG performance win.

 But.

 I'm having trouble getting it working with fixed-point
 math. So right now we're running floating point, and that
 sucks up most (all?) of the performance advantage, at least
 on machine with no or crappy fp hardware. (Works fine on 
 a PC!)

TODO:
 -- work out gains / scaling adjustments
 -- convert to fixed point in a manner that maintains precision
 
*/

DP_SUB_CREATOR_IMPL(fft_fir)

DP_FN_PREAMBLE(fft_fir,init) {
 s->filter_is_set = dp_false;
 s->filter_is_ready = dp_false;
 s->filter_is_complex = dp_false;
 s->fft_ready = dp_false;
 b->is_complex = dp_true;
 s->fft_len = 0;
 s->curr_shift = 0;
 b->complex_ok = dp_true;
 b->sub_work   = &dp_fft_fir_work;
 b->sub_deinit = &dp_fft_fir_deinit;
 b->sub_prerun = &dp_fft_fir_prerun;
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(fft_fir,make_unready) {
 if (s->fft_ready) {
  if (b->is_complex) {
   free(s->input_buf_c);
   free(s->output_buf_c);
   free(s->last_output_buf_c);
  } else {
   free(s->input_buf_r);
   free(s->output_buf_r);
   free(s->last_output_buf_r);
  }
  if (s->filter_is_ready) {
   free(s->filt_buf);
   s->filter_is_ready = dp_false;
  }
  free(s->fdom_buf);
  free(s->fdom_conv_buf);
  kiss_fft_cleanup();
  s->fft_ready= dp_false;
 }
}
DP_FN_POSTAMBLE


DP_FN_PREAMBLE(fft_fir,deinit) {
 dp_fft_fir_make_unready(b);
}
DP_FN_POSTAMBLE


void
dp_fft_fir_set_filter(dp_base_t *b, dp_int_t *nt, uint32_t ntlen, dp_bool_t c) {
 dp_fft_fir_sub_t *s = b->sub;
 if (DP_SUBVALID(b,fft_fir)) {
  s->filter_is_complex = c;
  s->filter_inptr      = nt;
  s->filter_inlen      = ntlen;
  s->filter_is_set     = dp_true;
  s->filter_is_ready   = dp_false;
 }
}


DP_FN_PREAMBLE(fft_fir,prepare_filter) {
 kiss_fft_cpx *filt_buf_in;	
 uint32_t i;
 float absmax =0;
 float scale_factor;

 kiss_fft_cfg filter_cfg;

 s->fft_len = b->is_complex ?
	      kiss_fft_next_fast_size(b->runlength + s->filter_inlen) :
	      kiss_fftr_next_fast_size_real(b->runlength+s->filter_inlen);

 if (s->filter_is_ready) {
  dp_fft_fir_make_unready(b);
 }

 filt_buf_in = malloc(sizeof(kiss_fft_cpx) * s->fft_len);
 s->filt_buf = malloc(sizeof(kiss_fft_cpx) * s->fft_len);
 
 if (s->filter_is_complex) {
  for (i=0;i<s->filter_inlen;i++) {
   filt_buf_in[i].i = s->fft_len * s->filter_inptr[2*i];
   filt_buf_in[i].r = s->fft_len * s->filter_inptr[2*i+1];
  }
 } else {
  for (i=0;i<s->filter_inlen;i++) {
   filt_buf_in[i].r = s->fft_len * s->filter_inptr[i];
   filt_buf_in[i].i = 0;
  }
 }

 for (i=s->filter_inlen;i<s->fft_len;i++) {
  filt_buf_in[i].r = 0;
  filt_buf_in[i].i = 0;
 }
 
 filter_cfg = kiss_fft_alloc(s->fft_len,0,0,0);
 kiss_fft(filter_cfg,filt_buf_in,s->filt_buf);

 absmax = 0;
 for (i=0;i<s->fft_len;i++) {
  if (s->filt_buf[i].i >  absmax) { absmax = s->filt_buf[i].i; }
  if (s->filt_buf[i].i < -absmax) { absmax = -s->filt_buf[i].i; }
  if (s->filt_buf[i].r >  absmax) { absmax = s->filt_buf[i].r; }
  if (s->filt_buf[i].r < -absmax) { absmax = -s->filt_buf[i].r; }
 }

 scale_factor = 32767.0 / absmax;
 for (i=0;i<s->fft_len;i++) {
  s->filt_buf[i].r = (float)s->filt_buf[i].r * scale_factor;
  s->filt_buf[i].i = (float)s->filt_buf[i].i * scale_factor;
 }

 s->filter_is_ready = dp_true;

 free(filt_buf_in);
}
DP_FN_POSTAMBLE



DP_FN_PREAMBLE(fft_fir,prerun) {
 if (s->filter_is_set) {
  dp_fft_fir_prepare_filter(b);
  if (!s->fft_ready && s->filter_is_ready) {
   if (b->is_complex) { 
    s->fft_fcfg_c    = kiss_fft_alloc(s->fft_len,0,0,0);
    s->fft_icfg_c    = kiss_fft_alloc(s->fft_len,1,0,0);
    s->input_buf_c   = malloc(sizeof(kiss_fft_cpx) * s->fft_len);
    s->fdom_buf      = malloc(sizeof(kiss_fft_cpx) * s->fft_len);
    s->fdom_conv_buf = malloc(sizeof(kiss_fft_cpx) * s->fft_len);
    s->output_buf_c  = malloc(sizeof(kiss_fft_cpx) * s->fft_len);
    s->last_output_buf_c = malloc(sizeof(kiss_fft_cpx)*(s->fft_len-b->runlength));
    memset(s->last_output_buf_c,0,sizeof(kiss_fft_cpx)*(s->fft_len-b->runlength));
   } else {
    s->fft_fcfg_r    = kiss_fftr_alloc(s->fft_len,0,0,0);
    s->fft_icfg_r    = kiss_fftr_alloc(s->fft_len,1,0,0);
    s->input_buf_r   = malloc(sizeof(kiss_fft_scalar)*(s->fft_len));
    s->fdom_buf      = malloc(sizeof(kiss_fft_cpx)*(s->fft_len/2+1));
    s->fdom_conv_buf = malloc(sizeof(kiss_fft_cpx)*(s->fft_len/2+1));
    s->output_buf_r  = malloc(sizeof(kiss_fft_scalar)*s->fft_len);
    s->last_output_buf_r = malloc(sizeof(kiss_fft_scalar)*(s->fft_len-b->runlength));
    memset(s->last_output_buf_r,0,sizeof(kiss_fft_scalar)*(s->fft_len-b->runlength));
   }
   s->fft_ready = dp_true;
  }
 }
}
DP_FN_POSTAMBLE


DP_FN_PREAMBLE(fft_fir,work) {
 if (!s->fft_ready) {
  dp_fft_fir_prerun(b);
 }

 if (b->is_complex) {
  dp_fft_fir_work_complex(b);
 } else {
  dp_fft_fir_work_real(b);
 }
}
DP_FN_POSTAMBLE


DP_FN_PREAMBLE(fft_fir,work_complex) {
 uint32_t i, extra;
 for (i=0;i<s->fft_len;i++) {
  s->input_buf_c[i].i = (i<b->runlength) ? s->fft_len * b->in_v->v[2*i]   : 0;
  s->input_buf_c[i].r = (i<b->runlength) ? s->fft_len * b->in_v->v[2*i+1] : 0;
 }

 kiss_fft(s->fft_fcfg_c, s->input_buf_c, s->fdom_buf);

 /* now we convolve with the filter. 
 // the filter buffer is a fraction of i16's, the fdom buffer is i32's */
 if (1) {
  for (i=0;i<s->fft_len;i++) {
   int64_t i_i = s->fdom_buf[i].i; 
   int64_t i_r = s->fdom_buf[i].r;
   int64_t f_i = s->filt_buf[i].i;
   int64_t f_r = s->filt_buf[i].r; 
   int64_t p_r = (i_r * f_r - i_i * f_i);
   int64_t p_i = (i_r * f_i + i_i * f_r);
   s->fdom_conv_buf[i].i = p_i >> 15;
   s->fdom_conv_buf[i].r = p_r >> 15;
  }
 }

 kiss_fft(s->fft_icfg_c,s->fdom_conv_buf,s->output_buf_c);

 extra = s->fft_len - b->runlength;
 for (i=0; i<b->runlength; i++) {
  float vi = s->output_buf_c[i].i;
  float vr = s->output_buf_c[i].r;
  vi += (i<extra) ? s->last_output_buf_c[i].i : 0;
  vr += (i<extra) ? s->last_output_buf_c[i].r : 0;
  b->out_v->v[2*i]     = vi;
  b->out_v->v[2*i+1]   = vr;
 }

 for (i=0;i<extra;i++) {
  s->last_output_buf_c[i] = s->output_buf_c[i+b->runlength];
 }
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(fft_fir,work_real) {
 dp_bool_t test_mode = dp_false;
 uint32_t i, extra;
 for (i=0;i<s->fft_len;i++) {
  s->input_buf_r[i] = (i<b->runlength) ? b->in_v->v[i] * s->fft_len : 0;
 }

 kiss_fftr(s->fft_fcfg_r, s->input_buf_r, s->fdom_buf);

 /* now we convolve with the filter. 
 // This should be fraction * fraction = fraction... one would hope */
 if (test_mode) {
  for (i=0;i<s->fft_len/2+1;i++) {
    s->fdom_buf[i].i *= 1;
    s->fdom_buf[i].r *= 1;
  }
 } else {
  for (i=0;i<(s->fft_len/2+1);i++) {
   int64_t i_i = s->fdom_buf[i].i;
   int64_t i_r = s->fdom_buf[i].r;
   int64_t f_i = s->filt_buf[i].i;
   int64_t f_r = s->filt_buf[i].r; 
   int64_t p_r = (i_r * f_r - i_i * f_i);
   int64_t p_i = (i_r * f_i + i_i * f_r);
   s->fdom_conv_buf[i].i = p_i >> 15;
   s->fdom_conv_buf[i].r = p_r >> 15;
  }
 }

 kiss_fftri(s->fft_icfg_r,s->fdom_conv_buf,s->output_buf_r);

 extra = s->fft_len - b->runlength;
 for (i=0; i<b->runlength; i++) {
  int32_t vr = s->output_buf_r[i];
  vr += (i<extra) ? s->last_output_buf_r[i] : 0;
  b->out_v->v[i]   = vr;
 }

 for (i=0;i<extra;i++) {
  s->last_output_buf_r[i] = s->output_buf_r[i+b->runlength];
 }
}
DP_FN_POSTAMBLE


uint32_t powerTwoGreater(uint32_t in) {
 uint32_t out = 0x1;
 uint32_t in_c = in;
 while (in) {
  in >>= 1;
  if (in_c != out) { 
   out <<= 1;
  }
 }
 return out;
}



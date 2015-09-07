
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz


#include "fft_filt.h"
#include "kiss_fft.h"
#include "kiss_fftr.h"

#include <iostream>

#include "c2r.h"

namespace djdsp {

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

fft_filt_c::fft_filt_c() {
 filter_is_set = false;
 filter_is_ready = false;
 filter_is_complex = false;
 fft_ready = false;
 is_complex = true;
 fft_len = 0;
 curr_shift = 0;
 complex_ok = true;
};

void
fft_filt_c::make_unready() {
 if (fft_ready) {
  if (is_complex) {
   delete input_buf_c;
   delete output_buf_c;
   delete last_output_buf_c;
  } else {
   delete input_buf_r;
   delete output_buf_r;
   delete last_output_buf_r;
  }
  if (filter_is_ready) {
   delete filt_buf;
   filter_is_ready = false;
  }
  delete fdom_buf;
  delete fdom_conv_buf;
  kiss_fft_cleanup();
  fft_ready= false;
 }

}
fft_filt_c::~fft_filt_c() {
 make_unready();
}

void fft_filt_c::set_filter(const int16_t *nt, uint32_t ntlen, sig_cpx_t s) {
 filter_is_complex = s == s_complex;
 filter_inptr      = nt;
 filter_inlen      = ntlen;
 filter_is_set     = true;
 filter_is_ready   = false;
};


void fft_filt_c::prepare_filter() {
	
 fft_len = is_complex ? kiss_fft_next_fast_size(l + filter_inlen) :
	                kiss_fftr_next_fast_size_real(l+filter_inlen);

 if (filter_is_ready) {
  make_unready();
 }

 kiss_fft_cpx *filt_buf_in = new kiss_fft_cpx[fft_len];
 filt_buf = new kiss_fft_cpx[fft_len];
 
 float absmax =0;
 if (filter_is_complex) {
  for (uint32_t i=0;i<filter_inlen;i++) {
   filt_buf_in[i].i = fft_len * filter_inptr[2*i];
   filt_buf_in[i].r = fft_len * filter_inptr[2*i+1];
  }
 } else {
  for (uint32_t i=0;i<filter_inlen;i++) {
   filt_buf_in[i].r = fft_len * filter_inptr[i];
   filt_buf_in[i].i = 0;
  }
 }

 for (uint32_t i=filter_inlen;i<fft_len;i++) {
  filt_buf_in[i].r = 0;
  filt_buf_in[i].i = 0;
 }
 
 kiss_fft_cfg filter_cfg = kiss_fft_alloc(fft_len,0,0,0);
 kiss_fft(filter_cfg,filt_buf_in,filt_buf);

 absmax = 0;
 for (uint32_t i=0;i<fft_len;i++) {
  if (filt_buf[i].i >  absmax) { absmax = filt_buf[i].i; };
  if (filt_buf[i].i < -absmax) { absmax = -filt_buf[i].i; };
  if (filt_buf[i].r >  absmax) { absmax = filt_buf[i].r; };
  if (filt_buf[i].r < -absmax) { absmax = -filt_buf[i].r; };
 }

 // for (uint32_t i=0;i<fft_len/2+1;i++) { d3c(i,filt_buf[i].i,filt_buf[i].r); };

 float scale_factor = 32767.0 / absmax;
 for (uint32_t i=0;i<fft_len;i++) {
  filt_buf[i].r = (float)filt_buf[i].r * scale_factor;
  filt_buf[i].i = (float)filt_buf[i].i * scale_factor;
 }

 // for (uint32_t i=0;i<fft_len/2+1;i++) { d3c(i,filt_buf[i].i,filt_buf[i].r); };

 filter_is_ready = true;

 delete filt_buf_in;
};



void fft_filt_c::pre_run() {
 if (filter_is_set) {
  prepare_filter();
  if (!fft_ready && filter_is_ready) {
   if (is_complex) {
    fft_fcfg_c    = kiss_fft_alloc(fft_len,0,0,0);
    fft_icfg_c    = kiss_fft_alloc(fft_len,1,0,0);
    input_buf_c   = new kiss_fft_cpx[fft_len];
    fdom_buf      = new kiss_fft_cpx[fft_len];
    fdom_conv_buf = new kiss_fft_cpx[fft_len];
    output_buf_c  = new kiss_fft_cpx[fft_len];
    last_output_buf_c = new kiss_fft_cpx[fft_len-l];
    memset(last_output_buf_c,0,sizeof(kiss_fft_cpx)*(fft_len-l));
   } else {
    fft_fcfg_r    = kiss_fftr_alloc(fft_len,0,0,0);
    fft_icfg_r    = kiss_fftr_alloc(fft_len,1,0,0);
    input_buf_r   = new kiss_fft_scalar[fft_len];
    fdom_buf      = new kiss_fft_cpx[fft_len/2+1];
    fdom_conv_buf = new kiss_fft_cpx[fft_len/2+1];
    output_buf_r  = new kiss_fft_scalar[fft_len];
    last_output_buf_r = new kiss_fft_scalar[fft_len-l];
    memset(last_output_buf_r,0,sizeof(kiss_fft_scalar)*(fft_len-l));
   }
   fft_ready = true;
  }
 }
};


void fft_filt_c::work() {
 if (!fft_ready) {
  pre_run();
 }

 if (is_complex) {
  work_complex();
 } else {
  work_real();
 }
}

void
fft_filt_c::work_complex() {
 for (uint32_t i=0;i<fft_len;i++) {
  input_buf_c[i].i = (i<l) ? fft_len * (*in)[2*i]   : 0;
  input_buf_c[i].r = (i<l) ? fft_len * (*in)[2*i+1] : 0;
 };

 kiss_fft(fft_fcfg_c, input_buf_c, fdom_buf);

 // now we convolve with the filter. 
 // the filter buffer is a fraction of i16's, the fdom buffer is i32's
 if (1) {
  for (uint32_t i=0;i<fft_len;i++) {
   int64_t i_i = fdom_buf[i].i; 
   int64_t i_r = fdom_buf[i].r;
   int64_t f_i = filt_buf[i].i;
   int64_t f_r = filt_buf[i].r; 
   int64_t p_r = (i_r * f_r - i_i * f_i);
   int64_t p_i = (i_r * f_i + i_i * f_r);
   fdom_conv_buf[i].i = p_i >> 15;
   fdom_conv_buf[i].r = p_r >> 15;
  }
 }

 kiss_fft(fft_icfg_c,fdom_conv_buf,output_buf_c);

 uint32_t extra = fft_len - l;
 for (uint32_t i=0; i<l; i++) {
  float vi = output_buf_c[i].i;
  float vr = output_buf_c[i].r;
  vi += (i<extra) ? last_output_buf_c[i].i : 0;
  vr += (i<extra) ? last_output_buf_c[i].r : 0;
  (*out)[2*i]     = vi;
  (*out)[2*i+1]   = vr;
 }

 for (uint32_t i=0;i<extra;i++) {
  last_output_buf_c[i] = output_buf_c[i+l];
 }
};

void
fft_filt_c::work_real() {
 for (uint32_t i=0;i<fft_len;i++) {
  input_buf_r[i] = (i<l) ? (*in)[i] * fft_len : 0;
 };

 kiss_fftr(fft_fcfg_r, input_buf_r, fdom_buf);

 // for (uint32_t i=0;i<fft_len/2+1;i++) { d3(i,fdom_buf[i].r,fdom_buf[i].i); }
  
 // now we convolve with the filter. 
 // This should be fraction * fraction = fraction... one would hope
 const bool test_mode = false;
 if (test_mode) {
  for (uint32_t i=0;i<fft_len/2+1;i++) {
    fdom_buf[i].i *= 1;
    fdom_buf[i].r *= 1;
  }
 } else {
  for (uint32_t i=0;i<(fft_len/2+1);i++) {
   int64_t i_i = fdom_buf[i].i;
   int64_t i_r = fdom_buf[i].r;
   int64_t f_i = filt_buf[i].i;
   int64_t f_r = filt_buf[i].r; 
   int64_t p_r = (i_r * f_r - i_i * f_i);
   int64_t p_i = (i_r * f_i + i_i * f_r);
   fdom_conv_buf[i].i = p_i >> 15;
   fdom_conv_buf[i].r = p_r >> 15;
  }
 }

 // for (uint32_t i=0;i<fft_len/2+1;i++) { d7c(i,filt_buf[i].r,filt_buf[i].i, fdom_buf[i].r,fdom_buf[i].i, fdom_conv_buf[i].r, fdom_conv_buf[i].i); };

 kiss_fftri(fft_icfg_r,fdom_conv_buf,output_buf_r);

 // for (uint32_t i=0;i<fft_len/2+1;i++) { d3c(i,(*in)[i],output_buf_r[i]); };

 uint32_t extra = fft_len - l;
 for (uint32_t i=0; i<l; i++) {
  int32_t vr = output_buf_r[i];
  vr += (i<extra) ? last_output_buf_r[i] : 0;
  (*out)[i]   = vr;
 }

 for (uint32_t i=0;i<extra;i++) {
  last_output_buf_r[i] = output_buf_r[i+l];
 }
};

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
};


} // namespace




#if 0

void
fft_filt_c::design_bpf(uint32_t sample_rate, float t1_start, float t1_end, float t2_start, float t2_end) {
 uint32_t buffer_len = l;
 fft_len             = kiss_fft_next_fast_size(buffer_len+60);

 filt_buf = new kiss_fft_cpx[fft_len];
 for (uint32_t i=0;i<fft_len;i++) {
  int32_t index = (i < (fft_len/2)) ? -i : fft_len - i;
  float   freq  = (float)index * (float)sample_rate / (float)fft_len;
  float   afreq = abs(freq);
  if (afreq < t1_start) {
   filt_buf[i].r = 0;
  } else if (afreq < t1_end) {
   filt_buf[i].r = 32767 * (1.0 - (t1_end - afreq) / (t1_end - t1_start));
  } else if (afreq < t2_start) {
   filt_buf[i].r = 32767;
  } else if (afreq < t2_end) {
   filt_buf[i].r = 32767 * (0.0 + (afreq - t2_start) / (t2_end - t2_start));
  } else {
   filt_buf[i].r = 0;
  }
  filt_buf[i].i = 0;
 }
 filter_is_set = true;
 if (fft_ready) {
  make_unready();
 }
 if (!name.compare("bpf_var30")) {
  for (uint32_t i=0; i< fft_len;i++) {
   std::cout << "filt[" << i << "] = " << filt_buf[i].r << ", " << filt_buf[i].i << std::endl;
  }
 }
};

void
fft_filt_c::design_lpf(uint32_t sample_rate, float t_start, float t_end) {
 uint32_t buffer_len = l;
 fft_len             = kiss_fft_next_fast_size(buffer_len+60);

 filt_buf = new kiss_fft_cpx[fft_len];
 for (uint32_t i=0;i<fft_len;i++) {
  int32_t index = (i < (fft_len/2)) ? -i : fft_len - i;
  float   freq  = (float)index * (float)sample_rate / (float)fft_len;
  float   afreq = abs(freq);
  if (afreq < t_start) {
   filt_buf[i].r = 32767;
  } else if (afreq < t_end) {
   filt_buf[i].r = 32767 * (t_end - afreq) / (t_end - t_start);
  } else {
   filt_buf[i].r = 0;
  }
  filt_buf[i].i = 0;
 }
 filter_is_set = true;
 if (fft_ready) {
  make_unready();
 }
 if (!name.compare("lpf_var30_1")) {
  for (uint32_t i=0; i< fft_len;i++) {
   std::cout << "filt[" << i << "] = " << filt_buf[i].r << ", " << filt_buf[i].i << std::endl;
  }
 }
};

#endif

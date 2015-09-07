
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>

#include "cmix.h"
#include "text_dump.h"

namespace djdsp {


// Implements a complex mixer class. Takes an IQ signal and performs
// a complex multiply with a sin/cos complex signal generated to match
// a given LO frequency provided. The LO frequency can be positive or 
// negative to slide the spectrum up or down. To slide up, provide a 
// negative value. 

cmixer_c::cmixer_c() {
 ready = false;
 sine_len = 0;
 sine_allocd = 0;
 sin_wave = 0;
 cos_wave = 0;
 cos_start = 0;
 curr_sine_idx = 0;
};

cmixer_c::~cmixer_c() {
 if (sine_allocd) {
  delete sin_wave;
  delete cos_wave;
 };
};

bool
cmixer_c::set_lo_freq(float lf) {
 lo_freq = lf;

 float abs_lo_freq            = fabs(lo_freq);
 float sine_len_exact         = (float)sample_rate / abs_lo_freq;
 float sine_repetitions_exact = (float)SINE_ALLOC / sine_len_exact;
 uint32_t sine_repetitions    = floor(sine_repetitions_exact);

 ready = false;
 return (sine_repetitions == 0);
};

void
cmixer_c::set_lo_amp(float la) {
 lo_amp = la;
 ready = false;
};

void
cmixer_c::set_sample_rate(uint32_t sr) {
 sample_rate = sr;
 ready = false;
};


uint32_t
cmixer_c::make_sine() {
 float abs_lo_freq            = fabs(lo_freq);
 float sine_len_exact         = (float)sample_rate / abs_lo_freq;
 float sine_repetitions_exact = (float)SINE_ALLOC / sine_len_exact;
 uint32_t sine_repetitions    = floor(sine_repetitions_exact);

 if (!sine_repetitions) {
  std::cout << "-error- cmixer_c (" << name << ") not enough samples in SINE_ALLOC to contain one full wave." << std::endl;
  exit(-1);
 } else {
  std::cout << "-info- cmixer_c (" << name << ") created LO waveform f=" 
	    << lo_freq << std::endl;
 }
 sine_len    = int(sine_len_exact*(float)sine_repetitions+0.5);

 if (!sine_allocd) {
  uint32_t alloc_size = SINE_ALLOC;
  sin_wave = new int16_t[alloc_size];
  cos_wave = new int16_t[alloc_size];
  sine_allocd = alloc_size;
 }

 for (uint32_t i=0;i<sine_len;i++) {
  float angle = 2.0*PI *
	        (float)i *
		(float)abs_lo_freq /
		(float)sample_rate;

  float   s_val_f = sin(angle);
  int16_t s_val_i = 32767 * s_val_f;
  float   c_val_f = cos(angle);
  int16_t c_val_i = 32767 * c_val_f;
  sin_wave[i] = s_val_i;
  cos_wave[i] = c_val_i;
 };

 curr_sine_idx = 0;
 ready = true;
 return sine_len;
};


void cmixer_c::work() {

  uint32_t len = l;	

  if (!ready) {
   make_sine();
  };

  // d1(curr_sine_idx);
  bool reverse = lo_freq < 0;

  for (uint32_t i=0;i<len;i++) {
   int16_t in_i = (*in)[2*i] ;
   int16_t in_q = (*in)[2*i+1];
   uint32_t idx = curr_sine_idx++ % sine_len;

   int16_t lo_i = reverse ? sin_wave[idx] : cos_wave[idx];
   int16_t lo_q = reverse ? cos_wave[idx] : sin_wave[idx];

   // d2(lo_i,lo_q);

   int32_t res_i_32 = (in_q * lo_i) + (in_i * lo_q);
   int32_t res_q_32 = (in_q * lo_q) - (in_i * lo_i);
   int16_t res_q_16 = SATURATE30(res_q_32) >> 15;
   int16_t res_i_16 = SATURATE30(res_i_32) >> 15;
  
   // uint16_t res_i_16 = lo_i;
   // uint16_t res_q_16 = lo_q;

   (*out)[2*i] =   res_i_16;
   (*out)[2*i+1] = res_q_16;
   // d2(idx,res_i_16); 
  };
};

} // namespace

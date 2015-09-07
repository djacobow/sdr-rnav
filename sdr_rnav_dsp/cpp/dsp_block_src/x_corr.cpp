

// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz


#include "x_corr.h"
#include <stdio.h>
#include <limits.h>

// Implementation of auick and dirty real-valued correlation function.
// It's an alternative to the phase compare class which counts time 
// between zero crossings. This routine slides on signal over the other
// searching for the maximum correlation. The amount of sliding is the 
// phase difference.
//
// It can only provide integer samples of phase info as implemented


namespace djdsp {

x_corr_c::x_corr_c() { 
 in2 = 0;
 dl  = 0;
};

x_corr_c::~x_corr_c() {
 // nothing
}

void 
x_corr_c::set_ins(const dvec_t *i1, const dvec_t *i2) {
 in  = i1;
 in2 = i2;
};

void
x_corr_c::set_delay_len(uint32_t idl) {
 dl = idl;
};

void
x_corr_c::work() {
 min_v = INT_MAX;
 max_v = INT_MIN; 
 min_l = 0;
 max_l = 0;

 for (uint32_t n=0;n<dl;n++) {
  int32_t s = 0;
  for (uint32_t m=0;m<l;m++) {
   uint32_t in2idx = n+m;
   if (in2idx < l) {
    s += (*in)[m] * (*in2)[n+m];
   }
  }
  int32_t res = s;
  if (res > max_v) { max_l = n; max_v = res; };
  if (res < min_v) { min_l = n; min_v = res; };
  (*out)[n] = (int16_t)(res >> 16);
 }
}

uint32_t
x_corr_c::get_minarg() {
 return min_l;
}

uint32_t
x_corr_c::get_maxarg() {
 return max_l;
}

} // namespace

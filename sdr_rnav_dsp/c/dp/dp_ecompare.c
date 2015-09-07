

/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#include "dp_ecompare.h"
#include <math.h>

/* This quick-and-dirty class is in support of a quick-and-dirty
// attempt to determine if we are tuned to a VOR station at all and
// if a carrier is present. 
//
// This class facilitates that by providing the ratio (in dB) of 
// the energy of two signals. (the RMS power)
//
// The two signasl would normally be the AM demodulated signal
// filtered separately: once bandpassed around a place where a 
// signal is expected (like 30 Hz in our case) and another bandpassed
// around a place where a signal is not expected, like 100 Hz
// (best to use a non-integer multiple of the other so you don't 
// pick up harmonics). If the ratio is above some threshold, then
// you are likely not looking at noise and have a true signal.
*/

DP_SUB_CREATOR_IMPL(ecompare)

DP_FN_PREAMBLE(ecompare,init) {
 b->sub_work = &dp_ecompare_work;
}
DP_FN_POSTAMBLE

void dp_ecompare_set_in2(dp_base_t *b, const dp_vec_t *i) {
 DP_IFSUBVALID(b,ecompare) {
  s->in2 = i;
 }
}

void dp_ecompare_set_ins(dp_base_t *b, dp_vec_t *i1, dp_vec_t *i2) {
 DP_IFSUBVALID(b,ecompare) {
  b->in_v = i1;
  s->in2  = i2;
 }
}

void dp_ecompare_set_out(dp_base_t *b, float *o) { 
 DP_IFSUBVALID(b,ecompare) {
  s->outvar = o; 
 }
}

const float ln10 = 2.30258509299;

DP_FN_PREAMBLE(ecompare,work) {
  uint64_t ss1 = 0;
  uint64_t ss2 = 0;
  uint32_t i;
  float a1, a2, ratio;
  for (i=0;i<b->runlength;i++) {
   int32_t s1 = b->in_v->v[i];
   int32_t s2 = s->in2->v[i];
   ss1 += s1 * s1;
   ss2 += s2 * s2;
  };
  ss1 /= b->runlength;
  ss2 /= b->runlength;
  a1 = sqrt((float)ss1);
  a2 = sqrt((float)ss2);
  ratio = a1 / a2; 
  *s->outvar = 20 * log(ratio) / ln10;
}
DP_FN_POSTAMBLE

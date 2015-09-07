

// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#include "ecompare.h"
#include <cmath>

// This quick-and-dirty class is in support of a quick-and-dirty
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

namespace djdsp {

void ecompare_c::set_in_2(const dvec_t *i) { in2 = i; };
void ecompare_c::set_in_1(const dvec_t *i) { set_in(i); };
void ecompare_c::set_ins(const dvec_t *i, const dvec_t *j) { in = i; in2 = j; };
void ecompare_c::set_out(float *o) { outvar = o; };

const float ln10 = 2.30258509299;

void ecompare_c::work() {
  uint64_t ss1 = 0;
  uint64_t ss2 = 0;
  for (uint32_t i=0;i<l;i++) {
   int32_t s1 = (*in)[i];
   int32_t s2 = (*in2)[i];
   ss1 += s1 * s1;
   ss2 += s2 * s2;
  };
  ss1 /= l;
  ss2 /= l;
  float a1 = sqrt((float)ss1);
  float a2 = sqrt((float)ss2);
  float ratio = a1 / a2; 
  *outvar = 20 * log(ratio) / ln10;
};

} // namespace

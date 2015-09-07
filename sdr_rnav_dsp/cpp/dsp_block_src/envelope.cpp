// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz

// This is an envelope follow class. It takes sampled audio data and
// converts it to an "envelope" that follows the overall contour of the
// amplitude at a much lower frequency.

#include "envelope.h"
#include <math.h>
#include <stdio.h>
#define DEFAULT_ATTACK  (0.010)
#define DEFAULT_RELEASE (0.010)
#include "helpers.h"

namespace djdsp {

int16_t
envelope_c::floatToFixed(float f) {
 if (f > 1)  { f = 1; };
 if (f < -1) { f = -1; };
 f *= 32768;
 return (int16_t)f;
};

int16_t
envelope_c::timeToCoeff(float t) {
 float g = exp(-1/((float)sample_rate*t));
 return (floatToFixed(g)); 
};


envelope_c::envelope_c() {
 attack_time = DEFAULT_ATTACK;
 release_time = DEFAULT_RELEASE;
 g_attack = timeToCoeff(attack_time);
 g_release = timeToCoeff(release_time);
 // fenv = 0;
 env = 0;
};


void envelope_c::set_sample_rate(uint16_t sr) {
 sample_rate = sr;
 g_attack = timeToCoeff(attack_time);
 g_release = timeToCoeff(release_time);
};

void envelope_c::set_attack_s(float a) {
 attack_time = a;
 g_attack = timeToCoeff(attack_time);
};

void envelope_c::set_release_s(float r) {
 release_time = r;
 g_release = timeToCoeff(release_time);
};

envelope_c::~envelope_c() {
 // nothing to do	
};

void envelope_c::work() {
 uint32_t len = l;

 for (uint32_t i=0;i<len;i++) {
  int32_t ein = abs((*in)[i]);
  if (env < ein) {
   env *= g_attack;
   env += (32768-g_attack)*ein;
  } else {
   env *= g_release;
   env += (32768-g_release)*ein;
  };
  env = SATURATE30(env);
  env >>= 15;
  (*out)[i] = (int16_t)env;
 };
};

} // namespace

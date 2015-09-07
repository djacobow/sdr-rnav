/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz

// This is an envelope follow class. It takes sampled audio data and
// converts it to an "envelope" that follows the overall contour of the
// amplitude at a much lower frequency.
*/

#include "dp_envelope.h"
#include <math.h>
#include <stdio.h>
#define DEFAULT_ATTACK  (0.010)
#define DEFAULT_RELEASE (0.010)

int16_t
floatToFixed(float f) {
 if (f > 1)  { f = 1; };
 if (f < -1) { f = -1; };
 f *= 32768;
 return (int16_t)f;
}

DP_SUB_CREATOR_IMPL(envelope)

int16_t
dp_envelope_timeToCoeff(dp_base_t *b, float t) {
 DP_IFSUBVALID(b,envelope) {
  float g = exp(-1/((float)s->sample_rate*t));
  return (floatToFixed(g)); 
 }
}


DP_FN_PREAMBLE(envelope,init) {
 s->attack_time = DEFAULT_ATTACK;
 s->release_time = DEFAULT_RELEASE;
 s->g_attack = dp_envelope_timeToCoeff(b,s->attack_time);
 s->g_release = dp_envelope_timeToCoeff(b,s->release_time);
 s->env = 0;
 b->sub_work = &dp_envelope_work;

}
DP_FN_POSTAMBLE



void 
dp_envelope_set_sample_rate(dp_base_t *b, uint16_t sr) {
 DP_IFSUBVALID(b,envelope) {
  s->sample_rate = sr;
  s->g_attack = dp_envelope_timeToCoeff(b,s->attack_time);
  s->g_release = dp_envelope_timeToCoeff(b,s->release_time);
 }
}

void 
dp_envelope_set_attack_s(dp_base_t *b, float a) {
 DP_IFSUBVALID(b,envelope) {
  s->attack_time = a;
  s->g_attack = dp_envelope_timeToCoeff(b,s->attack_time);
 }
}

void
dp_envelope_set_release_s(dp_base_t *b, float r) {
 DP_IFSUBVALID(b,envelope) {
 s-> release_time = r;
 s->g_release = dp_envelope_timeToCoeff(b,s->release_time);
 }
}

DP_FN_PREAMBLE(envelope,work) {
 uint32_t len = b->runlength;
 uint32_t i;
 for (i=0;i<len;i++) {
  int32_t ein = abs(b->in_v->v[i]);
  if (s->env < ein) {
   s->env *= s->g_attack;
   s->env += (32768-s->g_attack)*ein;
  } else {
   s->env *= s->g_release;
   s->env += (32768-s->g_release)*ein;
  };
  s->env = SATURATE30(s->env);
  s->env >>= 15;
  b->out_v->v[i] = (int16_t)s->env;
 }
}
DP_FN_POSTAMBLE

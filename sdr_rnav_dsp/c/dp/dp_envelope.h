
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/


#ifndef DP_ENVELOPE_H
#define DP_ENVELOPE_H

#include "dp_base_internals.h"

DP_SUB_CREATOR_DECL(envelope);
DP_FN_DECL(envelope,init);
DP_FN_DECL(envelope,work);
DP_SUB_GENERIC_SETTER_DECL(envelope,sample_rate,uint16_t);
DP_SUB_GENERIC_SETTER_DECL(envelope,attack_s,float);
DP_SUB_GENERIC_SETTER_DECL(envelope,release_s,float);

int16_t floatToFixed(float f);

int16_t dp_envelope_timeToCoeff(dp_base_t *, float);

typedef struct dp_envelope_sub_t {
  float attack_time;
  float release_time;
  int16_t g_attack;
  int16_t g_release;
  uint16_t sample_rate;
  int32_t env;
} dp_envelope_sub_t;

#endif



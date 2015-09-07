
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_QUASI_QUAD_H
#define DP_QUASI_QUAD_H

#include "dp_base_internals.h"

DP_SUB_CREATOR_DECL(quasiquad);
DP_FN_DECL(quasiquad,init);
DP_FN_DECL(quasiquad,work);
void dp_quasiquad_setup(dp_base_t *b, uint32_t sr, uint32_t carrier, uint32_t deviation);

typedef struct dp_quasiquad_sub_t {
  float last_s;
  float last_crossing;
  float curr_crossing;
  float period;
  float sample_rate;
  float past_avg_periods[3];
  int16_t multiplier;
  float trailing_avg_period;
  uint32_t carrier;
  uint32_t max_dev;
} dp_quasiquad_sub_t;

#endif


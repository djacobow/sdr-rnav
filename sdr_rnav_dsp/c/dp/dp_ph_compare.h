
/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef _DP_PH_COMPARE
#define _DP_PH_COMPARE

#include "dp_base_internals.h"


DP_SUB_CREATOR_DECL(ph_compare);
DP_FN_DECL(ph_compare,init);
DP_FN_DECL(ph_compare,work);

float dp_ph_compare_getLastBlockPhase(dp_base_t *b);
float dp_ph_compare_getLastBlockPeriod(dp_base_t *b, uint8_t which);
void dp_ph_compare_set_ins(dp_base_t *b, dp_vec_t *i1, dp_vec_t *i2);

typedef struct dp_ph_compare_sub_t {
  float curr_x_1;
  float curr_x_2;
  float last_x_1;
  float last_x_2;
  int16_t last_s_1;
  int16_t last_s_2;
  float last_block_p1;
  float last_block_p2;
  float last_block_delay;
  const dp_vec_t *in2;
} dp_ph_compare_sub_t;

#endif


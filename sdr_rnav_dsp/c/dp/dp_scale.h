/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_SCALE_H
#define DP_SCALE_H

#include "dp_base_internals.h"

DP_SUB_CREATOR_DECL(scale);
DP_FN_DECL(scale,work);
DP_FN_DECL(scale,init);

void dp_scale_set_int_iq(dp_base_t *, int16_t, int16_t);
void dp_scale_set_float_iq(dp_base_t *, double, double);
void dp_scale_set_int(dp_base_t *, int16_t);
void dp_scale_set_float(dp_base_t *, double);

typedef struct dp_scale_sub_t {
  int16_t scale_i;
  int16_t scale_q;
  dp_bool_t scale_set;
} dp_scale_sub_t;

#endif



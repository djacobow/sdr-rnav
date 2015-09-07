
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_DEINTERLEAVE_H 
#define DP_DEINTERLEAVE_H

#include "dp_base_internals.h"


DP_SUB_CREATOR_DECL(deinterleave);
DP_FN_DECL(deinterleave,init);
DP_FN_DECL(deinterleave,work);
DP_FN_DECL(deinterleave,deinit);
DP_SUB_GENERIC_SETTER_DECL(deinterleave,num_channels,uint8_t);
void dp_deinterleave_set_output(dp_base_t *, uint8_t pos, dp_vec_t *);

typedef struct dp_deinterleave_sub_t {
  uint8_t channel_count;
  dp_vec_t **outputs;
  dp_bool_t outs_set;
  dp_bool_t counts_set;
} dp_deinterleave_sub_t;


#endif



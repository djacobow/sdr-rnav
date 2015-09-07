
/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_INTERLEAVE_H 
#define DP_INTERLEAVE_H

#include "dp_base_internals.h"

DP_SUB_CREATOR_DECL(interleave);
DP_FN_DECL(interleave,init);
DP_FN_DECL(interleave,work);
DP_FN_DECL(interleave,deinit);
void dp_interleave_set_input(dp_base_t *, uint8_t, dp_vec_t *);
DP_SUB_GENERIC_SETTER_DECL(interleave,num_channels,uint8_t);

typedef struct dp_interleave_sub_t {
  uint8_t channel_count;
  dp_vec_t **inputs;
  dp_bool_t ins_set;
  dp_bool_t counts_set;
} dp_interleave_sub_t;

#endif



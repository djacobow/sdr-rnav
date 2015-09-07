
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_DC_BLOCK_H
#define DP_DC_BLOCK_H

#include "dp_base_internals.h"


DP_SUB_CREATOR_DECL(dcblock);
DP_FN_DECL(dcblock,init);
DP_FN_DECL(dcblock,work);
DP_SUB_GENERIC_SETTER_DECL(dcblock,pole,float);

typedef struct dp_dcblock_sub_t {
  int32_t acc, A, prev_x, prev_y;
  double pole;
  int16_t s_factor;
  int32_t hist[4];
} dp_dcblock_sub_t;

#endif




/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_THRESH_H
#define DP_THRESH_H

#ifndef DEFAULT_LO_THRESHOLD
#define DEFAULT_LO_THRESHOLD (225)
#endif

#ifndef DEFAULT_HI_THRESHOLD 
#define DEFAULT_HI_THRESHOLD (375)
#endif

#include "dp_base_internals.h"

DP_SUB_CREATOR_DECL(thresh);
DP_FN_DECL(thresh,init);
DP_FN_DECL(thresh,work);
DP_FN_DECL(thresh,rethresh);
DP_SUB_GENERIC_SETTER_DECL(thresh,autothresh,dp_bool_t);
void dp_thresh_set_thresholds(dp_base_t *b, int16_t l, int16_t h);

typedef struct dp_thresh_sub_t {
  int16_t lthresh;
  int16_t hthresh;
  int16_t last_val;
  int16_t c_max;
  int16_t c_min;
  int16_t l_c_max;
  int16_t l_c_min;
  dp_bool_t autothresh;
} dp_thresh_sub_t;

#endif



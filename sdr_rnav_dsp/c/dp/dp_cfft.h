
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_CFFT_H
#define DP_CFFT_H

#include "dp_base_internals.h"
#include "kiss_fft.h"

#ifndef PI
#define PI (3.14159265358)
#endif

DP_SUB_CREATOR_DECL(cfft);
DP_FN_DECL(cfft,init);
DP_FN_DECL(cfft,deinit);
DP_FN_DECL(cfft,prerun);
DP_FN_DECL(cfft,work);
DP_SUB_GENERIC_SETTER_DECL(cfft,averaging,float);

typedef struct dp_cfft_sub_t { 
  dp_bool_t    ready;
  kiss_fft_cfg cfg;
  dp_bool_t    buffs_allocd; 
  kiss_fft_cpx *ibuff;
  kiss_fft_cpx *obuff;
  dp_bool_t    need_buffers;
  uint8_t      averaging;
} dp_cfft_sub_t;

#endif


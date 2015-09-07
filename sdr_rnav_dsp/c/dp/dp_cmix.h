
/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_CMIX_H
#define DP_CMIX_H

#include "dp_base_internals.h"

#ifndef PI
#define PI (3.14159265358)
#endif

/* alloc up to this many samples to contain the sine waves. */
#define SINE_ALLOC (8*1024)

DP_SUB_CREATOR_DECL(cmix);
DP_FN_DECL(cmix,init);
DP_FN_DECL(cmix,work);
DP_FN_DECL(cmix,deinit);
DP_SUB_GENERIC_SETTER_DECL(cmix,sample_rate,uint32_t);
DP_SUB_GENERIC_SETTER_DECL(cmix,lo_amp,float);
dp_bool_t dp_cmix_set_lo_freq(dp_base_t *, float);
uint32_t dp_cmix_make_sine(dp_base_t *);

typedef struct dp_cmix_sub_t {
  uint32_t sample_rate;
  int16_t  *sin_wave;
  int16_t  *cos_wave;
  float    lo_freq;
  float    lo_amp;
  dp_bool_t ready;
  uint32_t sine_len;
  uint32_t sine_allocd;
  uint32_t cos_start;
  uint32_t curr_sine_idx;
} dp_cmix_sub_t;

#endif


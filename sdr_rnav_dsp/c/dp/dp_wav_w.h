/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/
#ifndef _DP_WAV_W_H
#define _DP_WAV_W_H

#include <stdio.h>
#include <stdlib.h>
#include "dp_base_internals.h"

typedef struct dp_wav_w_sub_t {
  dp_bool_t isopen;
  uint32_t sample_rate;
  uint16_t num_channels;
  uint16_t bits_per_sample;
  uint32_t bytes_written;
  FILE *fh;
  uint32_t ec;
  uint32_t last_elems;
  char *fname;
} dp_wav_w_sub_t;


DP_SUB_CREATOR_DECL(wav_w);
DP_FN_DECL(wav_w,work);
DP_FN_DECL(wav_w,prerun);
DP_FN_DECL(wav_w,init);
DP_FN_DECL(wav_w,postrun);
DP_FN_DECL(wav_w,deinit);
DP_FN_DECL(wav_w,close);

DP_SUB_GENERIC_SETTER_DECL(wav_w,sample_rate, uint32_t);
DP_SUB_GENERIC_SETTER_DECL(wav_w,num_channels, uint16_t);
DP_SUB_GENERIC_SETTER_DECL(wav_w,bits_per_sample, uint16_t);
DP_SUB_STRING_SETTER_DECL(wav_w,fname);
DP_SUB_GENERIC_GETTER_DECL(wav_w,last_elems, uint32_t);

#endif


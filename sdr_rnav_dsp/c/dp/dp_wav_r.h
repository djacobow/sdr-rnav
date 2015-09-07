/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/
#ifndef __DP_WAV_R_H
#define __DP_WAV_R_H

#include <stdio.h>
#include <stdlib.h>
#include "dp_base_internals.h"

typedef struct dp_wav_r_sub_t {
 FILE    *fh;
 dp_bool_t isopen;
 dp_bool_t iseof;
 dp_bool_t goodfile;
 uint32_t pos;
 uint32_t sample_rate;
 uint16_t num_channels;
 uint16_t sample_size;
 uint16_t audioformat;
 uint16_t blockalign;
 uint8_t  ec;
 uint32_t total_data_bytes;
 uint32_t last_elems;
 uint32_t subchunk2size;
 char    *fname;
} dp_wav_r_sub_t;

DP_SUB_CREATOR_DECL(wav_r);
DP_FN_DECL(wav_r,init);
DP_FN_DECL(wav_r,work);
DP_FN_DECL(wav_r,prerun);
DP_FN_DECL(wav_r,postrun);
DP_FN_DECL(wav_r,deinit);

DP_SUB_GENERIC_GETTER_DECL(wav_r,sample_rate,uint32_t);
DP_SUB_GENERIC_GETTER_DECL(wav_r,num_channels,uint8_t);
DP_SUB_GENERIC_GETTER_DECL(wav_r,sample_size,uint8_t);
DP_SUB_GENERIC_GETTER_DECL(wav_r,last_elems,uint32_t);
DP_SUB_STRING_SETTER_DECL(wav_r,fname);

#endif


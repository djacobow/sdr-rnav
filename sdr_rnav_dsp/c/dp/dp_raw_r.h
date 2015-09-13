/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Fall 2015
//
// Copyright 2013-5, David Jacobowitz
*/
#ifndef __DP_RAW_R_H
#define __DP_RAW_R_H

#include <stdio.h>
#include <stdlib.h>
#include "dp_base_internals.h"

typedef struct dp_raw_r_sub_t {
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
 char    *fname;
 float   *floatbuf;
} dp_raw_r_sub_t;

DP_SUB_CREATOR_DECL(raw_r);
DP_FN_DECL(raw_r,init);
DP_FN_DECL(raw_r,work);
DP_FN_DECL(raw_r,prerun);
DP_FN_DECL(raw_r,postrun);
DP_FN_DECL(raw_r,deinit);

DP_SUB_GENERIC_GETTER_DECL(raw_r,sample_rate,uint32_t);
DP_SUB_GENERIC_GETTER_DECL(raw_r,num_channels,uint8_t);
DP_SUB_GENERIC_GETTER_DECL(raw_r,sample_size,uint8_t);
DP_SUB_GENERIC_GETTER_DECL(raw_r,last_elems,uint32_t);
DP_SUB_STRING_SETTER_DECL(raw_r,fname);

#endif


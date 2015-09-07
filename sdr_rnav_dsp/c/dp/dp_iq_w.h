/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/
#ifndef _DP_IQ_W_H
#define _DP_IQ_W_H

#include <stdio.h>
#include <stdlib.h>
#include "dp_base_internals.h"

typedef struct dp_iq_w_sub_t {
  dp_bool_t isopen;
  uint16_t num_channels;
  uint32_t bytes_written;
  FILE *fh;
  uint32_t ec;
  uint32_t last_elems;
  char *fname;
} dp_iq_w_sub_t;


DP_SUB_CREATOR_DECL(iq_w);
DP_FN_DECL(iq_w,work);
DP_FN_DECL(iq_w,prerun);
DP_FN_DECL(iq_w,init);
DP_FN_DECL(iq_w,postrun);
DP_FN_DECL(iq_w,deinit);
DP_FN_DECL(iq_w,close);

DP_SUB_GENERIC_SETTER_DECL(iq_w,num_channels, uint16_t);
DP_SUB_STRING_SETTER_DECL(iq_w,fname);
DP_SUB_GENERIC_GETTER_DECL(iq_w,last_elems, uint32_t);

#endif


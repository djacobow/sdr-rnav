
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_TEXT_DUMP_H
#define DP_TEXT_DUMP_H

#include "dp_base_internals.h"
#include <stdio.h>

typedef struct dp_text_sub_t {
  FILE *fh;	  
  dp_bool_t isopen;
  uint8_t channels;
  char *fname;
  char *header;
  uint32_t ec;
  uint32_t iter;
} dp_text_sub_t;

DP_SUB_CREATOR_DECL(text);
DP_FN_DECL(text,init);
DP_FN_DECL(text,work);
DP_FN_DECL(text,prerun);
DP_FN_DECL(text,deinit);
DP_FN_DECL(text,postrun);
DP_SUB_STRING_SETTER_DECL(text,fname);
DP_SUB_STRING_SETTER_DECL(text,header);
DP_SUB_GENERIC_SETTER_DECL(text,channels,uint8_t);
void dp_text_iput(dp_base_t *b, const int16_t *buf, uint32_t len, uint8_t channels); 
void dp_text_fput(dp_base_t *b, const float *buf, uint32_t len, uint8_t channels); 
void dp_text_bput(dp_base_t *b, const dp_bool_t *buf, uint32_t len, uint8_t channels); 

#endif


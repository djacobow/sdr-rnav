/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz
*/
#include "dp_decimate.h"

/* This is a very simple decimation. Not moving average, just dropping
   samples. It only works for integer relationships. It's not a resampler.*/

DP_SUB_CREATOR_IMPL(decimate)

DP_FN_PREAMBLE(decimate,init) {
 b->sub_work = &dp_decimate_work;
 b->complex_ok = 1;
 b->is_complex = 0;
 s->decim = 1; 
}	  
DP_FN_POSTAMBLE

DP_SUB_GENERIC_SETTER_IMPL(decimate,decim,uint32_t)

DP_FN_PREAMBLE(decimate,work) {
 if (b->is_complex) {
  dp_decimate_cdecimate(b);
 } else {
  dp_decimate_decimate(b);
 }
}
DP_FN_POSTAMBLE


void dp_decimate_decimate(dp_base_t *b) {
 uint32_t len = b->runlength;
 uint32_t max_i = 0;
 uint32_t i;
 uint32_t idx_in;
 dp_decimate_sub_t *s = b->sub;
 for (i=0;i<len/s->decim;i++) {
  idx_in  = i*s->decim;
  b->out_v->v[i] = b->in_v->v[idx_in];
  if (i>max_i) { max_i = i; }
 }
}


/* complex version. Same as regular version but manages the I,Q stride
   of the arrays */
void dp_decimate_cdecimate(dp_base_t *b) {
 dp_decimate_sub_t *s = b->sub;
 uint32_t len = b->runlength;
 uint32_t i;
 for (i=0;i<len/s->decim;i++) {
  uint32_t idx_in  = 2*i*s->decim;
  uint32_t idx_out = 2*i;
  b->out_v->v[idx_out]   = b->in_v->v[idx_in ];
  b->out_v->v[idx_out+1] = b->in_v->v[idx_in+1];
 }
}


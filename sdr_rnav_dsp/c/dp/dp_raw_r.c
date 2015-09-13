/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz

// A class to provide quick and dirty read access to Microsoft ".raw" 
// files. 
//
// How is this quick and dirty:
//  1. almost no error checking
//  2. works on little-endian machines only. That's x86 and ARM, but 
//  if you're porting you'll probably get bitten.
//  2. the code is full of magic numbers, etc. 
*/

#include "dp_raw_r.h"
#include <stdio.h>
#include <stdlib.h>

DP_SUB_CREATOR_IMPL(raw_r)
DP_SUB_STRING_SETTER_IMPL(raw_r,fname)
DP_SUB_GENERIC_GETTER_IMPL(raw_r,sample_rate,uint32_t)
DP_SUB_GENERIC_GETTER_IMPL(raw_r,num_channels,uint8_t)
DP_SUB_GENERIC_GETTER_IMPL(raw_r,sample_size,uint8_t)
DP_SUB_GENERIC_GETTER_IMPL(raw_r,last_elems,uint32_t)

void
dp_raw_r_init(dp_base_t *b) {
 if (DP_SUBVALID(b,raw_r)) {
  b->sub_work    = &dp_raw_r_work;
  b->sub_prerun  = &dp_raw_r_prerun;
  b->sub_deinit  = &dp_raw_r_deinit;
 }
}


void dp_raw_r_deinit(dp_base_t *b) {
 dp_raw_r_sub_t *s;
 if (DP_VALID(b)) {
  s = b->sub;
  if (s->fname) {
   free(s->fname);
  }
  if (s->isopen) {
   fclose(s->fh);
  }
  b->sub_valid = 0;
 }
}



void dp_raw_r_prerun(dp_base_t *b) {
 dp_raw_r_sub_t *s;

 if (DP_SUBVALID(b,raw_r)) {
  s = b->sub;
  s->fh = fopen(s->fname, "rb");
  if (s->fh) {
   s->isopen = 1;
   s->pos = 0;
   s->iseof = 0;

   s->num_channels = 2;
   s->sample_rate = 192000;
   s->sample_size = 32;

   s->goodfile = 1;
#ifdef DEBUG
   fprintf(stderr,"-ci- raw_r (%s) opened file %s\n",
		   b->name,s->fname);
#endif
   s->iseof = feof(s->fh);
   s->floatbuf = malloc(sizeof(float) * b->runlength * s->num_channels);
   return;
  } else {
   s->ec |= 0x20;
  }
 }
}

void
dp_raw_r_work(dp_base_t *b) {
 dp_raw_r_sub_t *s = b->sub;
 uint32_t actual = 0;
 uint32_t count = b->runlength;

 int16_t *d = b->out_v->v;
 if (s->ec) {
  x1(s->ec); exit(-1);
 }
 switch (s->sample_size) {
  case 32 :
   actual = fread(s->floatbuf, 4, count*s->num_channels, s->fh); break;
  default :
   actual = 0;
 }
 uint32_t i;
 for (i=0;i<count*s->num_channels;i++) {
  float f = s->floatbuf[i];
  if ((f > 1.0) || (f < -1.0)) printf("f is %f\n",f);
  d[i] = f * 32767;
 }

 if (feof(s->fh)) {
  s->iseof = 1;
 }
 s->total_data_bytes += actual * s->num_channels * (s->sample_size / 8);
 s->last_elems = actual / s->num_channels;
}

void dp_raw_r_postrun(dp_base_t *b) {
 dp_raw_r_sub_t *s = b->sub;
 free(s->floatbuf);
}

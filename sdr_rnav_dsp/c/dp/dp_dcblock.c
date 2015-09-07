

/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#include <stdio.h>

#include "dp_dcblock.h"

/* Implements a simple HPF DC-blocker. Actually, the current implementation
// is not right, but works well enough. I "DC block" by subtracting an offset
// equal to a moveing average of the signal over the buffer (and last few 
// buffers) provided.
//
// This gets the job done, roughly.
*/


/* from http://www.dspguru.com/dsp/tricks/fixed-point-dc-blocking-filter-with-noise-shaping
// http://www.dsprelated.com/dspbooks/filters/DC_Blocker.html
*/

DP_SUB_CREATOR_IMPL(dcblock)

int16_t
dp_dcblock_adjust(dp_base_t *b) {
 DP_IFSUBVALID(b,dcblock) {
  s->A      = 32768.0*(1.0-s->pole);
  s->acc    = 0;
  s->prev_x = 0;
  s->prev_y = 0;
  s->s_factor = s->pole*(float)32767;
 }
 return s->s_factor;
}

DP_FN_PREAMBLE(dcblock,init) {
 int i;
 s->pole   = 0.99;
 for (i=0;i<4;i++) { s->hist[i] = 0; };
 dp_dcblock_adjust(b);
 b->sub_work = &dp_dcblock_work;
}
DP_FN_POSTAMBLE


void
dp_dcblock_set_pole(dp_base_t *b, float p) {
 DP_IFSUBVALID(b,dcblock) {
  if ((p > 0.9) && (p<1)) {
   s->pole = p;
  } else {
   fprintf(stderr,"-error pole must be bettween 0.9 and 1\n");
   exit(-1);
  }
 }
}

/*
 * This is my pathetic DC blocking hack. Just subtract the average 
 * of the block. To make it a bit more sophisticated, it takes out
 * the weighted average of this block and the last three, with 
 * decreasing weight.
 */
DP_FN_PREAMBLE(dcblock,work) {
  int32_t q = 0;
  uint32_t n;
  int32_t adj_amt;

  for (n=0;n<b->runlength;n++) {
   q += b->in_v->v[n];
  }
  q /= b->runlength;
  adj_amt = (q/2) + (s->hist[0]/4) + (s->hist[1]+s->hist[2])/8;
  for (n=0;n<b->runlength;n++) {
   b->out_v->v[n] = b->in_v->v[n] - adj_amt;
  }

  /* q-= adj_amt; */
  s->hist[3] = s->hist[2];
  s->hist[2] = s->hist[1];
  s->hist[1] = s->hist[0];
  s->hist[0] = q;
}
DP_FN_POSTAMBLE


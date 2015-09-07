
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#include "dp_ph_compare.h"
#include <stdio.h>

/* Implements a vrery sketchy (but cheap) phase comparator. It works
// by counting the time from a zero crossing in one signal to another
// zero crossing in the other signal. The assumption is that both signals
// are the same shape and frequency, which in the case of the VOR receiver
// they are. It also assumes that the signals are not too noisy, though
// one would hope that noise causing an early transition in one cycle
// would cause a late one later, and average out ok. We'll see if that 
// works.
*/

DP_SUB_CREATOR_IMPL(ph_compare)

DP_FN_PREAMBLE(ph_compare,init) {
 s->last_x_1 = 0;
 s->last_x_2 = 0;
 s->curr_x_1 = 0;
 s->curr_x_2 = 0;
 s->last_s_1 = 0;
 s->last_s_2 = 0;
 b->sub_work = &dp_ph_compare_work;
}
DP_FN_POSTAMBLE


void 
dp_ph_compare_set_ins(dp_base_t *b, dp_vec_t *i1, dp_vec_t *i2) {
 dp_ph_compare_sub_t *s = b->sub;
 if (DP_SUBVALID(b,ph_compare)) {
  b->in_v = i1;
  s->in2  = i2;
 }
}

float
dp_ph_compare_getLastBlockPhase(dp_base_t *b) {
 dp_ph_compare_sub_t *s = b->sub;
 if (DP_SUBVALID(b,ph_compare)) {
  return s->last_block_delay;
 }
 return -9999;
}

float
dp_ph_compare_getLastBlockPeriod(dp_base_t *b, uint8_t which) {
 dp_ph_compare_sub_t *s = b->sub;
 if (DP_SUBVALID(b,ph_compare)) {
  return which ? s->last_block_p2 : s->last_block_p1;
 } else {
  return -9999;
 }
}


DP_FN_PREAMBLE(ph_compare,work) {

 uint32_t curr_delay_ct = 0;
 float    curr_delay    = 0;
 uint32_t p1_ct         = 0;
 uint32_t p2_ct         = 0;
 float    p1_tot        = 0;
 float    p2_tot        = 0;
 uint32_t i;
 float    avg_delay;
 float    p1_avg, p2_avg;

 s->curr_x_1 = s->curr_x_1 - b->runlength;
 s->curr_x_2 = s->curr_x_2 - b->runlength;
 s->last_x_1 = s->last_x_1 - b->runlength;
 s->last_x_2 = s->last_x_2 - b->runlength;

 for (i=0;i<b->runlength;i++) {
  int16_t curr_s_1 = b->in_v->v[i];
  int16_t curr_s_2 = s->in2->v[i];

  if ((curr_s_1 >=0) && (s->last_s_1 < 0)) {
   p1_tot += s->curr_x_1 - s->last_x_1;  
   p1_ct++;

   s->last_x_1 = s->curr_x_1;
   s->curr_x_1 = i - (float)s->last_s_1/(float)(curr_s_1-s->last_s_1);
  }
  if ((curr_s_2 >=0) && (s->last_s_2 < 0)) {
   p2_tot += s->curr_x_2 - s->last_x_2;  
   p2_ct++;

   s->last_x_2 = s->curr_x_2;

   curr_delay += s->curr_x_2 - s->curr_x_1;
   curr_delay_ct++; 

   s->curr_x_2 = i - (float)s->last_s_2/(float)(curr_s_2-s->last_s_2);
  }
  s->last_s_1 = curr_s_1;
  s->last_s_2 = curr_s_2;
 }
 avg_delay = curr_delay / curr_delay_ct;
 p1_avg = p1_tot / p1_ct;
 p2_avg = p2_tot / p2_ct;
 s->last_block_p1 = p1_avg;
 s->last_block_p2 = p2_avg;
 s->last_block_delay = avg_delay;
}
DP_FN_POSTAMBLE
 

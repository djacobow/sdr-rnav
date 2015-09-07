/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz
*/

#include "dp_thresh.h"

DP_SUB_CREATOR_IMPL(thresh)

DP_FN_PREAMBLE(thresh,init) {
 s->lthresh = DEFAULT_LO_THRESHOLD;
 s->hthresh = DEFAULT_HI_THRESHOLD;
 s->autothresh = dp_false;
 s->last_val = 0;
 s->c_max = 0;
 s->c_min = 0x7fff;
 s->l_c_max = 0;
 s->l_c_min = 0x7fff;
 b->sub_work = &dp_thresh_work;
}
DP_FN_POSTAMBLE


void 
dp_thresh_set_thresholds(dp_base_t *b, int16_t l, int16_t h) {
 DP_IFSUBVALID(b, thresh) {
  s->lthresh = l;
  s->hthresh = h;
  s->autothresh = dp_false;
 }
}

DP_SUB_GENERIC_SETTER_IMPL(thresh,autothresh,dp_bool_t)

DP_FN_PREAMBLE(thresh,rethresh) {
 s->c_max = -32767;
 s->c_min = 32767;
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(thresh,work) {
 uint32_t len  = b->runlength;
 uint32_t i;
 int32_t max, min;
 int16_t range;
 dp_bool_t nv;

 /*
 s->c_max = -32767;
 s->c_min = 32767;
 */


 for (i=0;i<len;i++) {
  int16_t bval = b->in_v->v[i];

  /* Here's a bit of highly questionable code. Tries to guess what the
  // "high" and "low" thresholds for overcoming hysteresis should be
  // from the sample flow. Here we base it on the all-time high and 
  // low values in the input stream -- probably not a good idea in the 
  // presence of noise. Better plan is to track max and min over some 
  // number of samples
  */
  if (s->autothresh) {
   if (bval > s->c_max) { s->c_max = bval; }
   if (bval < s->c_min) { s->c_min = bval; }
   max = (s->c_max + s->l_c_max) >> 1;
   min = (s->c_min + s->l_c_min) >> 1;
   range = max - min;
   if (range<=0) { 
    range = 0;
    s->lthresh = s->c_min + 50;
    s->hthresh = s->c_max - 50;
   } else {   
    s->lthresh = min + 0.2 * range; /* was 0.4 7/14/2013 */
    s->hthresh = max - 0.6 * range; /* was 0.4 7/14/2013 */
   }
  }
  nv = (bval > s->hthresh) ? 0x1 : (bval < s->lthresh) ? 0x0 : s->last_val;

  b->out_v->v[i] = nv;
  s->last_val = nv;
 }
 s->l_c_max = s->c_max;
 s->l_c_min = s->c_min;
}
DP_FN_POSTAMBLE



/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#include "dp_quasiquad.h"
#include <stdio.h>

/*
 
Implementation of a "hack" quadrature demodulator that may only work in
this application.

We need to do quadrature demodulations in order to extract the 30 Hz
signal from  the 9960 Hz / 480 Hz deviation subcarrier. However, that's
hard, so we're going to do period counting by zero crossing instead --
which will hopefully work if the  signal is not too noisy!

 This routine kinda works now on my sample file, but it has a few
problems:   
 - need to get an fixed point version working   
 - would be nice if it didn't take the first few frames to figure 
   out the average variation and thus settle out. Maybe will just use 
   the expected variation for this calculation. That is, we know that 
   the 9960 Hz subcarrier will have an average period of sample_rate/9960 
   and a maximum of sample_rate/10440 and minimum of sample_rate/9480, 
   so we can use those values to scale the signal.
 - noise causes random crossings and "pops" in the output. How do we fix that?

*/



DP_SUB_CREATOR_IMPL(quasiquad)

DP_FN_PREAMBLE(quasiquad,init) {
 uint8_t i;
 s->last_s = 0;
 s->last_crossing = 0;
 s->curr_crossing = 0;
 s-> sample_rate = 44100;
 for (i=0;i<3;i++) { s->past_avg_periods[i] = 0; };
 s->multiplier = 30000;
 s->trailing_avg_period = 0;
 b->sub_work = &dp_quasiquad_work;
}
DP_FN_POSTAMBLE




void
dp_quasiquad_setup(dp_base_t *b, uint32_t sr, uint32_t carr, uint32_t mdev) {
 DP_IFSUBVALID(b,quasiquad) {
  uint8_t i;
  float min_c_period, max_c_period, avg_c_period;
  s->sample_rate = sr;
  s->carrier      = carr;
  s->max_dev     = mdev;

  min_c_period = s->sample_rate / (s->carrier+s->max_dev);
  max_c_period = s->sample_rate / (s->carrier-s->max_dev);
  avg_c_period = s->sample_rate / (s->carrier);

  s->trailing_avg_period = avg_c_period;
  for (i=0;i<3;i++) { s->past_avg_periods[i] = avg_c_period; };
  s->multiplier = (max_c_period-min_c_period) * 60000;
 }
}	

DP_FN_PREAMBLE(quasiquad,work) {
 float avg_period = 0;
 uint32_t pcount = 0;
 uint32_t i;
 s->last_crossing = s->last_crossing - b->runlength;
 for (i=0;i<b->runlength;i++) {
  float curr_s = b->in_v->v[i];
  if ((curr_s >=0) && (s->last_s < 0)) {
   s->curr_crossing = (float)i + ((float)(0-s->last_s)/(float)(curr_s-s->last_s));
   s->period = s->curr_crossing - s->last_crossing;
   avg_period += s->period;
   pcount++;
   s->last_crossing = s->curr_crossing;
  }
  b->out_v->v[i] = s->multiplier * (s->period - s->trailing_avg_period);
  s->last_s  = curr_s;
 };
 avg_period /= (float)pcount;
 s->past_avg_periods[2] = s->past_avg_periods[1];
 s->past_avg_periods[1] = s->past_avg_periods[0];
 s->past_avg_periods[0] = avg_period;
 s->trailing_avg_period = 
  ((4.0 * s->past_avg_periods[0] + 2.0 * s->past_avg_periods[1] + 2.0 * s->past_avg_periods[2])/8.0);
}
DP_FN_POSTAMBLE

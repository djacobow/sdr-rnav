
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#include <inttypes.h>
#include "quasi_quad.h"

#include <stdio.h>

namespace djdsp {

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



quasi_quad_c::quasi_quad_c() {
 last_s = 0;
 last_crossing = 0;
 curr_crossing = 0;
 sample_rate = 44100;
 for (uint8_t i=0;i<3;i++) { past_avg_periods[i] = 0; };
 multiplier = 30000;
 trailing_avg_period = 0;
};

quasi_quad_c::~quasi_quad_c() { };

void
quasi_quad_c::setup(uint32_t sr, uint32_t carr, uint32_t mdev) {
 sample_rate = sr;
 carrier      = carr;
 max_dev     = mdev;

 float min_c_period = sample_rate / (carrier+max_dev);
 float max_c_period = sample_rate / (carrier-max_dev);
 float avg_c_period = sample_rate / (carrier);

 trailing_avg_period = avg_c_period;
 for (uint8_t i=0;i<3;i++) { past_avg_periods[i] = avg_c_period; };
 multiplier = (max_c_period-min_c_period) * 60000;
}	

void
quasi_quad_c::work() {

 last_crossing = last_crossing - l;

 float avg_period = 0;
 uint32_t pcount = 0;
 for (uint32_t i=0;i<l;i++) {
  float curr_s = (*in)[i];
  if ((curr_s >=0) && (last_s < 0)) {
   curr_crossing = (float)i + ((float)(0-last_s)/(float)(curr_s-last_s));
   period = curr_crossing - last_crossing;
   avg_period += period;
   pcount++;
   last_crossing = curr_crossing;
  };
  (*out)[i] = multiplier * (period - trailing_avg_period);
  last_s  = curr_s;
 };
 avg_period /= (float)pcount;
 past_avg_periods[2] = past_avg_periods[1];
 past_avg_periods[1] = past_avg_periods[0];
 past_avg_periods[0] = avg_period;
 trailing_avg_period = 
  ((4.0 * past_avg_periods[0] + 2.0 * past_avg_periods[1] + 2.0 * past_avg_periods[2])/8.0);
};

} // namespace

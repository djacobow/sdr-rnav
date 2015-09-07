
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#include "ph_compare.h"
#include <stdio.h>

namespace djdsp {

// Implements a vrery sketchy (but cheap) phase comparator. It works
// by counting the time from a zero crossing in one signal to another
// zero crossing in the other signal. The assumption is that both signals
// are the same shape and frequency, which in the case of the VOR receiver
// they are. It also assumes that the signals are not too noisy, though
// one would hope that noise causing an early transition in one cycle
// would cause a late one later, and average out ok. We'll see if that 
// works.

ph_compare_c::ph_compare_c() { 
 last_x_1 = 0;
 last_x_2 = 0;
 curr_x_1 = 0;
 curr_x_2 = 0;
 last_s_1 = 0;
 last_s_2 = 0;
};

ph_compare_c::~ph_compare_c() { };


void 
ph_compare_c::set_ins(const dvec_t *i1, const dvec_t *i2) {
 in  = i1;
 in2 = i2;
};

float
ph_compare_c::getLastBlockPhase() {
 return last_block_delay;
}

float
ph_compare_c::getLastBlockPeriod(uint8_t which) {
 return which ? last_block_p2 : last_block_p1;
};



void
ph_compare_c::work() {

 curr_x_1 = curr_x_1 - l;
 curr_x_2 = curr_x_2 - l;
 last_x_1 = last_x_1 - l;
 last_x_2 = last_x_2 - l;

 uint32_t curr_delay_ct = 0;
 float    curr_delay    = 0;
 uint32_t p1_ct         = 0;
 uint32_t p2_ct         = 0;
 float    p1_tot        = 0;
 float    p2_tot        = 0;

 for (uint32_t i=0;i<l;i++) {
  int16_t curr_s_1 = (*in)[i];
  int16_t curr_s_2 = (*in2)[i];

  if ((curr_s_1 >=0) && (last_s_1 < 0)) {
   p1_tot += curr_x_1 - last_x_1;  
   p1_ct++;

   last_x_1 = curr_x_1;
   curr_x_1 = i - (float)last_s_1/(float)(curr_s_1-last_s_1);
   // printf("new x1: %f\n",curr_x_1);
  }
  if ((curr_s_2 >=0) && (last_s_2 < 0)) {
   p2_tot += curr_x_2 - last_x_2;  
   p2_ct++;

   last_x_2 = curr_x_2;

   curr_delay += curr_x_2 - curr_x_1;
   curr_delay_ct++; 

   curr_x_2 = i - (float)last_s_2/(float)(curr_s_2-last_s_2);
   // printf("new x2: %f\n",curr_x_2);
  }
  last_s_1 = curr_s_1;
  last_s_2 = curr_s_2;
 };
 float avg_delay = curr_delay / curr_delay_ct;
 float p1_avg = p1_tot / p1_ct;
 float p2_avg = p2_tot / p2_ct;
 last_block_p1 = p1_avg;
 last_block_p2 = p2_avg;
 last_block_delay = avg_delay;
}

 

} // namespace

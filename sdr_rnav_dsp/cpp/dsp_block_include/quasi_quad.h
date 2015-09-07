
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef _QUASI_QUAD_H
#define _QUASI_QUAD_H

#include "block_base.h"

namespace djdsp {

class quasi_quad_c : public block_base_c {
 public:
  quasi_quad_c();
  ~quasi_quad_c();
  void work();
  void setup(uint32_t sr, uint32_t carrier, uint32_t deviation);
 private:
  float last_s;
  float last_crossing;
  float curr_crossing;
  float period;
  float sample_rate;
  float past_avg_periods[3];
  int16_t multiplier;
  float trailing_avg_period;
  uint32_t carrier;
  uint32_t max_dev;

};

} // namespace

#endif



// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef _PH_COMPARE
#define _PH_COMPARE

#include "block_base.h"

namespace djdsp {

class ph_compare_c : public block_base_c {
 public:
  ph_compare_c();
  ~ph_compare_c();

  void set_ins(const dvec_t *i1, const dvec_t *i2);
  float getLastBlockPhase();
  float getLastBlockPeriod(uint8_t which);
  void  work();
 private:
  float curr_x_1;
  float curr_x_2;
  float last_x_1;
  float last_x_2;
  int16_t last_s_1;
  int16_t last_s_2;

  float last_block_p1;
  float last_block_p2;
  float last_block_delay;
  const dvec_t *in2;
};

} // namespace

#endif


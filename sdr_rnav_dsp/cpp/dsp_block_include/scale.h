// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz

#ifndef SCALE_H
#define SCALE_H

#include "block_base.h"

namespace djdsp {

class scale_c : public block_base_c {
 public:
  scale_c();
  void work();
  void set_scale_i(int16_t i, int16_t q);
  void set_scale_f(double i, double q);
  void set_scale_i(int16_t r);
  void set_scale_f(double r);
 private:
  int16_t scale_i;
  int16_t scale_q;
  bool scale_set;
};

} // namespace

#endif



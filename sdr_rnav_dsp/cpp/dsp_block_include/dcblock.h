
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef _DC_BLOCK_H
#define _DC_BLOCK_H

#include "block_base.h"

namespace djdsp {


class dcblock_c : public block_base_c {
 public:
  dcblock_c();
  ~dcblock_c();
  void set_pole(float p);
  void work();
 private:
  int32_t acc, A, prev_x, prev_y;
  double pole;
  int16_t adjust();
  int16_t s_factor;
  int32_t hist[4];
};

} // namespace

#endif


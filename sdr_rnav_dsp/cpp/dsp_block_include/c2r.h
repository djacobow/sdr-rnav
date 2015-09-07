
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef C2R_H
#define C2R_H

#include "block_base.h"

namespace djdsp {

uint32_t SquareRoot(uint32_t a_nInput);
int16_t comp16tomag16(int16_t r, int16_t i);

class c2r_c : public block_base_c {
 public:
  c2r_c();
  void work();
};

} // namespace

#endif



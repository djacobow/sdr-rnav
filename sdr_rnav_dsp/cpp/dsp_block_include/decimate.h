
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef _DECIMATE_H
#define _DECIMATE_H

#include "block_base.h"


namespace djdsp {

class decimate_c : public block_base_c {
 public:
  decimate_c();
  ~decimate_c();
  void set_decim(uint32_t decim);
  void work();
 private:
  uint32_t decim;
  void decimate();
  void cdecimate();
};

} // namespace

#endif


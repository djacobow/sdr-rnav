
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef CFFT_H
#define CFFT_H

#include "block_base.h"
#include "kiss_fft.h"

#ifndef PI
#define PI (3.14159265358)
#endif

namespace djdsp {

class cfft_c : public block_base_c {

 public:
  cfft_c();
  ~cfft_c();
  void work();
  void pre_run();
  void set_averaging(float f);

 private:
  bool     ready;
  kiss_fft_cfg cfg;
  bool     buffs_allocd; 
  kiss_fft_cpx *ibuff;
  kiss_fft_cpx *obuff;
  bool     need_buffers;
  uint8_t  averaging;
};


} // namespace

#endif


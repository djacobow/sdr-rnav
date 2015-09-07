
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef CMIX_H
#define CMIX_H

#include "block_base.h"

#ifndef PI
#define PI (3.14159265358)
#endif

// alloc up to this many samples to contain the sine waves.
#define SINE_ALLOC (8*1024)

namespace djdsp {

class cmixer_c : public block_base_c {

 public:
  cmixer_c();
  ~cmixer_c();
  bool set_lo_freq(float lf);
  void set_lo_amp(float la);
  void set_sample_rate(uint32_t sr);
  void work();

 private:
  uint32_t make_sine();
  uint32_t sample_rate;
  int16_t  *sin_wave;
  int16_t  *cos_wave;
  float    lo_freq;
  float    lo_amp;
  bool     ready;
  uint32_t sine_len;
  uint32_t sine_allocd;
  uint32_t cos_start;
  uint32_t curr_sine_idx;
};

} // namespace


#endif


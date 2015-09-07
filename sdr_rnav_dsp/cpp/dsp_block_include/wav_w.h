
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef _BAD_WAV_W_H
#define _BAD_WAV_W_H

#include "block_base.h"

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

namespace djdsp {

class wav_w_c : public block_base_c {

 public:
  wav_w_c();
  void set_sample_rate(uint32_t);
  void set_num_channels(uint8_t);
  void set_bits_per_sample(uint16_t);
  void set_file(const std::string);
  void pre_run();
  void work();
  uint32_t lastElems();
  void close();
  ~wav_w_c();
 private:
  bool isopen;
  uint32_t sample_rate;
  uint16_t num_channels;
  uint16_t bits_per_sample;
  uint32_t bytes_written;
  FILE *fh;
  uint32_t ec;
  uint32_t last_elems;
  std::string fname;
};

} // namespace

#endif


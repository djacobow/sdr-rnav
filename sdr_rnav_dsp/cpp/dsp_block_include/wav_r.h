
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef __BAD_WAV_R_H
#define __BAD_WAV_R_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "block_base.h"
#include <string>

namespace djdsp {

class wav_r_c : public block_base_c {

 private:
  FILE    *fh;
  uint8_t  valid;
  uint8_t  isopen;
  uint8_t  iseof;
  uint8_t  goodfile;
  uint32_t pos;
  uint32_t sample_rate;
  uint16_t num_channels;
  uint16_t  sample_size;
  uint16_t audioformat;
  uint16_t blockalign;
  uint8_t  ec;
  uint32_t total_data_bytes;
  uint32_t last_elems;
  uint32_t subchunk2size;
 public:
  wav_r_c();
  void     set_file(const std::string ifn);
  uint32_t get_sample_rate();
  uint16_t get_num_channels();
  uint16_t get_sample_size();
  uint16_t get_format();
  void     pre_run();
   
  // returns the number of time samples, not the number of bytes. For example,
  // if it returns one time sample of a four channel file with 16b samples,
  // the return value is 1. Similarly, count is in time samples, too.
  void     work();
  uint32_t lastElems();
  void     file_info_str(void);
  void     close();
  std::string fname;
  ~wav_r_c();
};

} // namespace

#endif


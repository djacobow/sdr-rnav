
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef _TEXT_DUMP_H
#define _TEXT_DUMP_H

#include "block_base.h"
#include <string>

namespace djdsp {

class text_dump_c : public block_base_c {
 private:
  FILE *fh;	  
  bool isopen;
  uint8_t channels;
  std::string fname;
  std::string header;
  uint32_t ec;
  uint32_t iter;
 public:
  text_dump_c();
  ~text_dump_c();
  void set_file(const std::string fn, const std::string hdr);
  void pre_run();
  void set_channels(uint8_t ch);
  void work();
  void close();
  void iput(const int16_t *buf, uint32_t len, uint8_t channels); 
  void fput(const float *buf, uint32_t len, uint8_t channels); 
  void bput(const bool *buf, uint32_t len, uint8_t channels); 
};

} // namespace

#endif



// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef _MORSEDEC_H
#define _MORSEDEC_H

#include "block_base.h"
#include <string>

namespace djdsp {

class morsedec_c : public block_base_c {
 public: 
  morsedec_c();
  ~morsedec_c();
  void work();
  std::string getDecoded();
 private:
  int8_t tpos;
  std::string decoded_str;
  uint32_t last_elems;
};


} // namespace

#endif


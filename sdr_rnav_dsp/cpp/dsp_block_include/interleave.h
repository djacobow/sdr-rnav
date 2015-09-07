
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz

#ifndef INTERLEAVE_H 
#define INTERLEAVE_H

#include "block_base.h"

namespace djdsp {

#ifndef INTERLEAVE_DVECS_T
#define INTERLEAVE_DVECS_T
typedef std::vector<dvec_t *> dvecs_t;
#endif

class interleave_c : public block_base_c {

 public:
  interleave_c();
  ~interleave_c();
  void set_num_channels(uint8_t ccount);
  void set_input(uint8_t pos, dvec_t *in);
 private:
  void work();
  uint8_t channel_count;
  dvecs_t inputs;
  bool ins_set;
  bool counts_set;
 
};

} // namespace

#endif



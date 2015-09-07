
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef DEINTERLEAVE_H 
#define DEINTERLEAVE_H

#include "block_base.h"

namespace djdsp {

#ifndef INTERLEAVE_DVECS_T
#define INTERLEAVE_DVECS_T
typedef std::vector<dvec_t *> dvecs_t;
#endif

class deinterleave_c : public block_base_c {
 public:
  deinterleave_c();
  ~deinterleave_c();
  void set_num_channels(uint8_t ccount);
  void set_output(uint8_t pos, dvec_t *out);
 private:
  void work();
  uint8_t channel_count;
  dvecs_t outputs;
  bool outs_set;
  bool counts_set;
 
};

} // namespace

#endif



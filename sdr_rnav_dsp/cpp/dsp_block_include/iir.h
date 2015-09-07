
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef __IIR_C_H
#define __IIR_C_H

#include "block_base.h"

namespace djdsp {

class iir_c : public block_base_c {

 public:
  iir_c();
  void set_filter(const double *newtaps, uint32_t newtlen);
  void work();
  ~iir_c();

 private:
  int8_t coeff_shift;
  dvec_t xbuffer;
  dvec_t ybuffer;
  dvec_t x_taps;
  dvec_t y_taps;
  i8vec_t x_shft;
  i8vec_t y_shft;

  uint32_t tlen;
  bool is_set;
  bool hb_is_set;
};

} // namespace

#endif


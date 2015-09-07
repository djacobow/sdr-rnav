
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef __FIR_C_H
#define __FIR_C_H

#include "block_base.h"

namespace djdsp {

class fir_c : public block_base_c {

 public:
  fir_c();
  void set_filter(const int16_t *newtaps, uint32_t newtlen, sig_cpx_t s);
  void work();
  void pre_run();
  ~fir_c();

 private:
  void prepare_filter();
  void work_single();
  void work_interleaved();
  dvec_t  taps;
  // dvec_t last_tlen1;
  // dvec_t last_tlen2;
  dvec_t hbuffer1;
  dvec_t hbuffer2;
  uint32_t tlen;
  const int16_t *filter_inptr;
  uint32_t filter_inlen;
  bool filter_is_set;
  bool filter_is_ready;
  bool hb_is_set;
  bool filter_is_complex;
};

} // namespace

#endif


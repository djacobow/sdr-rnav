
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef _X_CORR_H
#define _X_CORR_H

#include "block_base.h"

namespace djdsp {

class x_corr_c : public block_base_c {
 public:
  x_corr_c();
  ~x_corr_c();

  void set_ins(const dvec_t *i1, const dvec_t *i2);
  void set_delay_len(uint32_t idl);
  uint32_t get_minarg();
  uint32_t get_maxarg();
  void  work();
 private:
  const dvec_t *in2;
  uint32_t dl;
  int32_t max_v;
  int32_t min_v;
  uint32_t max_l;
  uint32_t min_l;
};

} // namespace

#endif


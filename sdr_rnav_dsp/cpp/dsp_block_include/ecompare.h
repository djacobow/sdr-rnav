
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef ECOMPARE_H
#define ECOMPARE_H

// Compare the energy in two signals

#include "block_base.h"

namespace djdsp {


class ecompare_c : public block_base_c {
 public:
  void work();
  void set_in_2(const dvec_t *i);
  void set_in_1(const dvec_t *i);
  void set_ins(const dvec_t *i, const dvec_t *j);
  void set_out(float *o);
 private:
  const dvec_t *in2;
  float *outvar;
};

} // namespace


#endif


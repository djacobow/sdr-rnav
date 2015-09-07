// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz

#include "multiply.h"
#include "c2r.h"

namespace djdsp {

multiply_c::multiply_c() {
 dvecs.resize(0);
 is_complex = false;
 complex_ok = true;
};

void
multiply_c::add_factor(dvec_t *i) {
 dvecs.push_back(i);
};

void
multiply_c::drop_factors() {
 dvecs.resize(0);
};

void
multiply_c::work() {
 int32_t p_r = 32767;
 int32_t p_i = 0;

 for (uint32_t i=0;i<l;i++) {
  for (uint8_t j=0;j<dvecs.size();j++) {
   dvec_t *v = dvecs[j];
   if (is_complex) {
    p_r = p_r * v->at(2*i)   - p_i * v->at(2*i+1);
    p_i = p_r * v->at(2*i+1) + p_i * v->at(2*i);
   } else {
    p_r *= v->at(i);
   }
   p_r = SATURATE30(p_r) >> 15;
   if (is_complex) {
    p_i = SATURATE30(p_i) >> 15;
   }
  }
  if (is_complex) {
   (*out)[2*i]   = p_r;
   (*out)[2*i+1] = p_i;
  } else {
   (*out)[i] = p_r;
  }
 };
};


} // namespace

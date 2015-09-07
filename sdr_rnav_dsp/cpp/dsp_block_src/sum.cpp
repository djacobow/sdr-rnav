
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz

#include "sum.h"
#include "c2r.h"
#include <iostream>

namespace djdsp {

sum_c::sum_c() {
 dvecs.resize(0);
 is_complex = false;
};

void
sum_c::add_addend(dvec_t *i, float scale) {
 sum_stream_t ns;
 ns.vector     = i;
 scale = (scale > 1.0) ? 1.0 : (scale < 0) ? 0 : scale;
 ns.scale_f    = scale;
 ns.scale_i    = scale * 32767.0; 
 dvecs.push_back(ns);
};

void
sum_c::drop_addends() {
 dvecs.resize(0);
};

void
sum_c::work() {

 for (uint32_t i=0;i<l;i++) {
  int32_t t_r = 0;
  int32_t t_i = 0;
  for (uint8_t j=0;j<dvecs.size();j++) {
   dvec_t *v = dvecs[j].vector;
   int32_t  scale_i = dvecs[j].scale_i;

   int32_t p_r = is_complex ? v->at(2*i)   : v->at(i);
   int32_t p_i = is_complex ? v->at(2*i+1) : 0; 

   int32_t s_r = p_r * scale_i;
   t_r += (SATURATE30(s_r) >> 15);
   if (is_complex) {
    int32_t s_i = p_i * scale_i;
    t_i += (SATURATE30(s_i) >> 15);
   }
  }
  
  if (is_complex) {
   (*out)[2*i]   = t_r;
   (*out)[2*i+1] = t_i;
  } else {
   (*out)[i]     = t_r;
  }
 };
};


} // namespace

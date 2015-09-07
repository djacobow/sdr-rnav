// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#include <math.h>
#include "c2r.h"
#include "helpers.h"

namespace djdsp {

// Implementation of complex IQ to magnitude routine

// Fast square root routine for fixed point.
// cribbed from 
// http://stackoverflow.com/questions/1100090/looking-for-an-efficient-integer-square-root-algorithm-for-arm-thumb2
uint32_t SquareRoot(uint32_t a_nInput) {
 uint32_t op  = a_nInput;
 uint32_t res = 0;
 uint32_t one = 1uL << 30; 
 while (one > op) {
        one >>= 2;
 }

 while (one != 0) {
  if (op >= res + one) {
   op = op - (res + one);
   res = res +  2 * one;
  }
  res >>= 1;
  one >>= 2;
 }
 return res;
};

c2r_c::c2r_c() {
 complex_ok = true;
 is_complex = true;
};

void 
c2r_c::work() {
 uint32_t len = l;	
 if (is_complex) {
  for (uint32_t i=0;i<len;i++) {
   int16_t in_i = (*in)[2*i];
   int16_t in_q = (*in)[2*i+1];
   (*out)[i] = comp16tomag16(in_i,in_q);
  };
 } else {
  for (uint32_t i=0;i<len;i++) {
   (*out)[i] = (*in)[i];
  }
 }
};


int16_t comp16tomag16(int16_t r, int16_t i) {
 int32_t in_r_sq = (int32_t)r * (int32_t)r;
 int32_t in_i_sq = (int32_t)i * (int32_t)i;
 int32_t m_sq    = in_r_sq + in_i_sq;
 m_sq    = SATURATE30(m_sq); 
 int16_t  mag = SquareRoot(m_sq);
 return mag; 
};

} // namespace

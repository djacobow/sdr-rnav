/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#include <math.h>
#include "dp_c2r.h"
#include "dp_helpers.h"

/*
// Implementation of complex IQ to magnitude routine

// Fast square root routine for fixed point.
// cribbed from 
// http://stackoverflow.com/questions/1100090/looking-for-an-efficient-integer-square-root-algorithm-for-arm-thumb2

*/

uint32_t dp_SquareRoot(uint32_t a_nInput) {
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
}

DP_SUB_CREATOR_IMPL(c2r)

DP_FN_PREAMBLE(c2r,init) {
 b->complex_ok = 1;
 b->is_complex = 1;
 b->sub_work   = &dp_c2r_work;
}
DP_FN_POSTAMBLE


DP_FN_PREAMBLE(c2r,work) {
 uint32_t len = b->runlength;	
 uint32_t i;
 if (b->is_complex) {
  for (i=0;i<len;i++) {
   int16_t in_i = b->in_v->v[2*i];
   int16_t in_q = b->in_v->v[2*i+1];
   b->out_v->v[i] = dp_comp16tomag16(in_i,in_q);
  }
 } else {
  for (i=0;i<len;i++) {
   b->out_v->v[i] = b->in_v->v[i];
  }
 }
}
DP_FN_POSTAMBLE


dp_int_t dp_comp16tomag16(dp_int_t r, dp_int_t i) {
 int32_t in_r_sq = (int32_t)r * (int32_t)r;
 int32_t in_i_sq = (int32_t)i * (int32_t)i;
 int32_t m_sq    = in_r_sq + in_i_sq;
 int16_t mag;
 m_sq    = SATURATE30(m_sq); 
 mag = dp_SquareRoot(m_sq);
 return mag; 
}


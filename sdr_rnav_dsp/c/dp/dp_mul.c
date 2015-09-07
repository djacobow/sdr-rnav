/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz
*/

#include "dp_mul.h"
#include "dp_c2r.h"
#include <stdio.h>

DP_SUB_CREATOR_IMPL(mul)

DP_FN_PREAMBLE(mul,init) {
 s->mul_vecs   = malloc(sizeof(dp_vec_t *) * MAX_FACTORS);
 s->vecs_ct    = 0;
 s->max_vecs   = MAX_FACTORS;
 b->is_complex = 0;
 b->complex_ok = 1;
 b->sub_work   = &dp_mul_work;
 b->sub_deinit = &dp_mul_deinit;
}
DP_FN_POSTAMBLE


DP_FN_PREAMBLE(mul,deinit) {
 free(s->mul_vecs);
 s->max_vecs = 0;
}
DP_FN_POSTAMBLE

dp_bool_t 
dp_mul_add_factor(dp_base_t *b, dp_vec_t *i) {
 dp_mul_sub_t *s = b->sub;
 if (DP_SUBVALID(b,mul)) {
  if (s->vecs_ct < s->max_vecs) {
   s->mul_vecs[s->vecs_ct++] = i;
   return 1;
  } else {
   fprintf(stderr,"-error- (%s) can't add another vector to multiplier.\n",b->name);
   return 0;
  }
 }
 return 0;
}


DP_FN_PREAMBLE(mul,work) {
 int32_t p_r = 32767;
 int32_t p_i = 0;
 uint32_t i, j;

 for (i=0;i<b->runlength;i++) {
  for (j=0;j<s->vecs_ct;j++) {
   dp_vec_t *v = s->mul_vecs[j];
   if (b->is_complex) {
    p_r = p_r * v->v[2*i]   - p_i * v->v[2*i+1];
    p_i = p_r * v->v[2*i+1] + p_i * v->v[2*i];
   } else {
    p_r *= v->v[i];
   }
   p_r = SATURATE30(p_r) >> 15;
   if (b->is_complex) {
    p_i = SATURATE30(p_i) >> 15;
   }
  }
  if (b->is_complex) {
   b->out_v->v[2*i]   = p_r;
   b->out_v->v[2*i+1] = p_i;
  } else {
   b->out_v->v[i] = p_r;
  }
 }
}
DP_FN_POSTAMBLE


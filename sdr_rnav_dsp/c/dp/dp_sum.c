/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz
*/

#include "dp_sum.h"
#include "dp_c2r.h"
#include <stdio.h>

DP_SUB_CREATOR_IMPL(sum)

DP_FN_PREAMBLE(sum,init) {
 s->sum_vecs   = malloc(sizeof(dp_sum_str_t) * MAX_ADDENDS);
 memset(s->sum_vecs,0,sizeof(dp_sum_str_t) * MAX_ADDENDS);
 s->vecs_ct    = 0;
 s->max_vecs   = MAX_ADDENDS;
 b->is_complex = 0;
 b->complex_ok = 1;
 b->sub_work   = &dp_sum_work;
 b->sub_deinit = &dp_sum_deinit;
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(sum,deinit) {
 s->max_vecs = 0;
 s->vecs_ct = 0;
 free(s->sum_vecs);
 s->sum_vecs = 0;
}
DP_FN_POSTAMBLE


dp_bool_t 
dp_sum_add_addend(dp_base_t *b, dp_vec_t *i, float scale) {
 dp_sum_sub_t *s = b->sub;
 if (DP_SUBVALID(b,sum)) {
  if (s->vecs_ct < s->max_vecs) {
   s->sum_vecs[s->vecs_ct].v = i;
   scale = (scale > 1.0) ? 1.0 : (scale < 0) ? 0 : scale;
   s->sum_vecs[s->vecs_ct].scale_f = scale;
   s->sum_vecs[s->vecs_ct].scale_i = scale * 32767.0;
   s->vecs_ct++;
   return 1;
  } else {
   fprintf(stderr,"-error- (%s) can't add another vector to summer.\n",b->name);
   return 0;
  }
 }
 return 0;
}


DP_FN_PREAMBLE(sum,work) {
 uint32_t i, j;

 for (i=0;i<b->runlength;i++) {
  int32_t t_r = 0;
  int32_t t_i = 0;
  for (j=0;j<s->vecs_ct;j++) {
   dp_vec_t *v = s->sum_vecs[j].v;
   int32_t  scale_i = s->sum_vecs[j].scale_i;

   int32_t p_r = b->is_complex ? v->v[2*i]   : v->v[i];
   int32_t p_i = b->is_complex ? v->v[2*i+1] : 0;

   int32_t s_r = p_r * scale_i;
   t_r += (SATURATE30(s_r) >> 15);
   if (b->is_complex) {
    int32_t s_i = p_i * scale_i;
    t_i += (SATURATE30(s_i) >> 15);
   }
  }

  if (b->is_complex) {
   b->out_v->v[2*i]   = t_r;
   b->out_v->v[2*i+1] = t_i;
  } else {
   b->out_v->v[i]     = t_r;
  }
 }
}
DP_FN_POSTAMBLE


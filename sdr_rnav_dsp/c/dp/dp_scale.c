#include "dp_scale.h"

#include <math.h>

DP_SUB_CREATOR_IMPL(scale)

DP_FN_PREAMBLE(scale,init) {
 b->is_complex = dp_false;
 s->scale_set  = dp_false;
 b->complex_ok = dp_true;
 b->sub_work   = &dp_scale_work;
}
DP_FN_POSTAMBLE

void
dp_scale_set_int_iq(dp_base_t *b, int16_t i, int16_t q) {
 dp_scale_sub_t *s = b->sub;
 if (DP_SUBVALID(b,scale)) {
  b->is_complex = dp_true;
  s->scale_i = i;
  s->scale_q = q;
  s->scale_set = dp_true;
 }
}

void
dp_scale_set_int(dp_base_t *b, int16_t i) {
 dp_scale_sub_t *s = b->sub;
 if (DP_SUBVALID(b,scale)) {
  s->scale_i = i;
  s->scale_q = 0;
  s->scale_set = dp_true;
  b->is_complex = dp_false;
 }
}

void
dp_scale_set_float(dp_base_t *b, double f) {
 dp_scale_sub_t *s = b->sub;
 if (DP_SUBVALID(b,scale)) {
  if (f<-1) { f = -1; }
  if (f>1)  { f = 1;  }
  f *= 32767.0;
  s->scale_i = f;
  s->scale_q = 0;
  s->scale_set = dp_true;
  b->is_complex = dp_false;
 }
}


void
dp_scale_set_float_iq(dp_base_t *b, double fi, double fq) {
 dp_scale_sub_t *s = b->sub;
 if (DP_SUBVALID(b,scale)) {
  double fm = sqrt(fi*fi+fq*fq);
  if (fm>1) {
   fi /= fm;
   fq /= fm;
  } 
  s->scale_i = fi * 32767.0;
  s->scale_q = fq * 32767.0;
  s->scale_set = dp_true;
  b->is_complex = dp_true;
 }
}


DP_FN_PREAMBLE(scale,work) {
 uint32_t i = 0; 
 if (s->scale_set) {
  if (b->is_complex) { 
   for (i=0;i<b->runlength;i++) {
    int32_t ii = b->in_v->v[2*i];
    int32_t iq = b->in_v->v[2*i+1];
    int32_t pi = ii * s->scale_i - iq * s->scale_q;
    int32_t pq = ii * s->scale_q + iq * s->scale_i;
    b->out_v->v[2*i]   = (SATURATE30(pi)) >> 15; 
    b->out_v->v[2*i+1] = (SATURATE30(pq)) >> 15; 
   }
  } else {
   for (i=0;i<b->runlength;i++) {
    int32_t p = b->in_v->v[i];
    p *= s->scale_i;
    b->out_v->v[i] = (SATURATE30(p)) >> 15;
   }
  }
 }
}
DP_FN_POSTAMBLE


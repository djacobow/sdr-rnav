/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz

// A straightforward and quasi-canonical but still probably compiler-
// unrollable implementation of fixed point FIR filters. Should work 
// fine as long as the number of taps in the filter is less than the 
// size of the buffer. Note that I do the filter in two pieces, a complicated
// bit and a simple bit. I could have done it in one piece, but I think
// this makes the less complicated bit (which is fine for most of the
// samples) more accessible to the compiler for optimization. Not that
// I've run with optimization turned on or looked at any assembly output
// yet...
*/

#include "dp_fir.h"
#include "dp_helpers.h"

DP_SUB_CREATOR_IMPL(fir)


DP_FN_PREAMBLE(fir,init) {
  b->complex_ok = 1;
  b->sub_work   = &dp_fir_work;
  b->sub_prerun = &dp_fir_prerun;
  b->sub_deinit = &dp_fir_deinit;
}
DP_FN_POSTAMBLE



DP_FN_PREAMBLE(fir,deinit) {
 dp_vec_deinit(&s->taps);
 dp_vec_deinit(&s->hbuffer1);
 dp_vec_deinit(&s->hbuffer2);
 /* will have more cleanup here */
 b->sub_valid = 0;
}
DP_FN_POSTAMBLE


void
dp_fir_set_filter(dp_base_t *b, dp_int_t *nt, uint32_t ntlen, dp_bool_t c) {
 dp_fir_sub_t *s;
 if (DP_SUBVALID(b,fir)) {
  s = b->sub;
  s->filter_is_complex = c;
  s->filter_inptr = nt;
  s->filter_inlen = ntlen;
  s->tlen = ntlen;
  s->filter_is_set = 1;
  s->filter_is_ready = 0;
 }
}

DP_FN_PREAMBLE(fir,prerun) {
 if (s->filter_is_set && !s->filter_is_ready) {
  dp_fir_prep_filter(b);
 }
}
DP_FN_POSTAMBLE


DP_FN_PREAMBLE(fir,prep_filter) {
 uint32_t i;
 s = b->sub;
 if (s->filter_is_set) {
  if (s->filter_is_complex) {
   dp_vec_resize(&s->taps,2*s->filter_inlen);
   for (i=0;i<(s->filter_inlen-1);i++) {
    s->taps.v[2*i]   = s->filter_inptr[2*i];
    s->taps.v[2*i+1] = s->filter_inptr[2*i+1];
   }
   s->taps.v[2*(s->filter_inlen-1)]   = s->filter_inptr[2*(s->filter_inlen-1)];
   s->taps.v[2*(s->filter_inlen-1)+1] = s->filter_inptr[2*(s->filter_inlen-1)+1];
  } else {
   dp_vec_resize(&s->taps,2*s->filter_inlen);
   for (i=0;i<(s->filter_inlen-1);i++) {
    s->taps.v[i] = s->filter_inptr[i];
   }
   s->taps.v[s->filter_inlen-1] = s->filter_inptr[s->filter_inlen-1];
  }
  s->filter_is_ready = 1;
  s->hb_is_set = 0;
 }
DP_FN_POSTAMBLE

}

DP_FN_PREAMBLE(fir,work) {
 if (!s->filter_is_ready && s->filter_is_set) {
  dp_fir_prep_filter(b);
 } else if (s->filter_is_ready) {
  if (b->is_complex) {
   dp_fir_work_interleaved(b);
  } else {
   dp_fir_work_single(b);
  }
 }
}
DP_FN_POSTAMBLE


void
dp_fir_work_single(dp_base_t *b) {
 uint32_t bcount = b->runlength;
 dp_fir_sub_t *s = b->sub;
 uint32_t tcount = s->tlen;
 uint32_t hcount = bcount + tcount;
 uint32_t i, k;
 int32_t acc = 0;
 uint32_t hidx; 

 if (!s->hb_is_set) {
  dp_vec_resize(&s->hbuffer1,hcount);
  for (i=0;i<hcount;i++) { s->hbuffer1.v[i] = 0; }
  s->hb_is_set = 1;
 }

 for (i=0;i<tcount;i++) {
  s->hbuffer1.v[i] = s->hbuffer1.v[bcount+i];
 }
 for (i=tcount;i<hcount;i++) {
  s->hbuffer1.v[i] = b->in_v->v[i-tcount];
 }

 for (i=0;i<bcount;i++) {
  acc = 1 << 14;
  for (k=0;k<tcount;k++) {
   hidx = tcount + i - k;
   acc += s->taps.v[k] * s->hbuffer1.v[hidx];
  }
  acc = SATURATE30(acc);
  b->out_v->v[i] = (int16_t)(acc >> 15);
 }
}


void
dp_fir_work_interleaved(dp_base_t *b) {
 uint32_t bcount = b->runlength;
 dp_fir_sub_t *s = b->sub;
 uint32_t tcount = s->tlen;
 uint32_t hcount = bcount + tcount;
 uint32_t i, k;
 int32_t acc1, acc2;
 uint32_t hidx;

 if (!s->hb_is_set) {
  dp_vec_resize(&s->hbuffer1,hcount);
  dp_vec_resize(&s->hbuffer2,hcount);
  for (i=0;i<hcount;i++) { 
   s->hbuffer1.v[i] = 0; 
   s->hbuffer2.v[i] = 0; 
  }
  s->hb_is_set = 1;

 }

 for (i=0;i<tcount;i++) {
  s->hbuffer1.v[i] = s->hbuffer1.v[bcount+i];
  s->hbuffer2.v[i] = s->hbuffer2.v[bcount+i];
 }
 for (i=tcount;i<hcount;i++) {
  s->hbuffer1.v[i] = b->in_v->v[2*(i-tcount)];
  s->hbuffer2.v[i] = b->in_v->v[2*(i-tcount)+1];
 }

 acc1 = 0;
 acc2 = 0;
 for (i=0;i<bcount;i++) {
  acc1 = 1 << 14;
  acc2 = 1 << 14;
  if (s->filter_is_complex) {
   for (k=0;k<tcount;k++) {
    hidx = tcount + i - k;
    acc1 += s->taps.v[2*k]   * s->hbuffer1.v[hidx] - s->taps.v[2*k+1] * s->hbuffer2.v[hidx];
    acc2 += s->taps.v[2*k+1] * s->hbuffer2.v[hidx] + s->taps.v[2*k]   * s->hbuffer1.v[hidx];
   }
  } else {
   for (k=0;k<tcount;k++) {
    hidx = tcount + i - k;
    acc1 += s->taps.v[k] * s->hbuffer1.v[hidx];
    acc2 += s->taps.v[k] * s->hbuffer2.v[hidx];
   }
  }
  acc1 = SATURATE30(acc1);
  acc2 = SATURATE30(acc2);
  b->out_v->v[(2*i)+0] = (int16_t)(acc1 >> 15);
  b->out_v->v[(2*i)+1] = (int16_t)(acc2 >> 15);
 }
}



/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "dp_cfft.h"
#include "kiss_fft.h"

/* Implements a complex mixer class. Takes an IQ signal and performs
// a complex multiply with a sin/cos complex signal generated to match
// a given LO frequency provided. The LO frequency can be positive or 
// negative to slide the spectrum up or down. To slide up, provide a 
// negative value. 
*/

DP_SUB_CREATOR_IMPL(cfft)

DP_FN_PREAMBLE(cfft,init) {
 s->ready = dp_false;
 s->buffs_allocd = dp_false;
 /* optimization removed for now; the fft can share 
 // buffers with the in/out except that I've decided
 // to implement averaging/smoothing, which makes that
 // not work, at least for the output buffer.
 // int iq_size = sizeof(kiss_fft_cpx);
 // int twosize = 2 * sizeof(myint_t);
 // need_buffers = twosize != iq_size;
 */
 s->need_buffers = 1;
 s->averaging = 0;
 b->complex_ok = dp_true;
 b->sub_work   = &dp_cfft_work;
 b->sub_prerun = &dp_cfft_prerun;
 b->sub_deinit = &dp_cfft_deinit;
}
DP_FN_POSTAMBLE


DP_FN_PREAMBLE(cfft,deinit) {
 if (s->buffs_allocd) {
  free(s->ibuff);
  free(s->obuff);
  kiss_fft_cleanup();
 }
}
DP_FN_POSTAMBLE

void
dp_cfft_set_averaging(dp_base_t *b, float f) {
 int mul_old;
 DP_IFSUBVALID(b,cfft) {
  if (f<0) { f = 0; };
  if (f>1) { f = 1; };

  mul_old = f * 256.0;
  if (mul_old > 255) {
   mul_old = 255;
  }
  s->averaging = mul_old;
 }
}

DP_FN_PREAMBLE(cfft,prerun) {
 if (!b->is_complex) {
  fprintf(stderr,"-ce-(%s) real ffn not supported yet\n",b->name);
  exit(-1);
 }

 if (s->need_buffers && !s->buffs_allocd) {
  fprintf(stderr,"-ci- cfft (%s) allocated buffer\n",b->name);
  s->ibuff = malloc(sizeof(kiss_fft_cpx) * b->runlength);
  s->obuff = malloc(sizeof(kiss_fft_cpx) * b->runlength);
  s->buffs_allocd = dp_true;
 }

 if (!s->ready) {
  s->cfg = kiss_fft_alloc(b->runlength,0,0,0);
  s->ready = dp_true;
 }
}
DP_FN_POSTAMBLE

/* Some optimization work needs to be done. This function 
// can work directly from the input and output buffers for the 
// object, but there are two downsides. 1) we'd still need
// another buffer to do smoothing and 2) the output order of the 
// fft is not convenient for visualization. It has 0 Hz at index 0
// rather than centered int the list. The (-1)^i flipping below
// fixes that, but requires another buffer.
*/
DP_FN_PREAMBLE(cfft,work) {
  uint32_t i;
  if (s->need_buffers && !s->buffs_allocd) {
   dp_cfft_prerun(b);
  }

  if (s->need_buffers) {
   for (i=0;i<b->runlength;i++) {
    /* making sure to get r and i parts in the right sequence
     * and also applying -i^i transform to make result in order
     * linear from -sr/2 to sr/2.
     */
    s->ibuff[i].r = b->in_v->v[2*i]    * ((i%2) ? -1 : 1);
    s->ibuff[i].i = b->in_v->v[2*i+1]  * ((i%2) ? -1 : 1);
   } 
  } else {
   s->ibuff = (kiss_fft_cpx *)(void *)b->in_v->v;
   s->obuff = (kiss_fft_cpx *)(void *)b->out_v->v;
  }

  kiss_fft(s->cfg,s->ibuff, s->obuff);

  if (s->need_buffers) {
   for (i=0;i<b->runlength;i++) {
    int32_t av_r, av_i;
    av_r = (255 - s->averaging) * s->obuff[i].r + s->averaging * b->out_v->v[2*i];
    av_i = (255 - s->averaging) * s->obuff[i].i + s->averaging * b->out_v->v[2*i+1];
    b->out_v->v[2*i]   = av_r >> 8;
    b->out_v->v[2*i+1] = av_i >> 8;
    /* printf("i,%d,fft,%d,%d,in,%d,%d\n",i,av_r>>8,av_i>>8,b->in_v->v[2*i],b->in_v->v[2*i+1]);  */
   }
  }
  /* printf("\n\n\n\n");  */
}
DP_FN_POSTAMBLE

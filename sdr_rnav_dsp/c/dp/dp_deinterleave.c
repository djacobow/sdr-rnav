#include "dp_deinterleave.h"

#include <stdio.h>


DP_SUB_CREATOR_IMPL(deinterleave)

DP_FN_PREAMBLE(deinterleave,init) {
 s->channel_count = 0;
 s->counts_set = dp_false;
 s->outs_set = dp_false;
 b->sub_work = &dp_deinterleave_work;
 b->sub_deinit = &dp_deinterleave_deinit;
}
DP_FN_POSTAMBLE


DP_FN_PREAMBLE(deinterleave,deinit) {
 if (s->counts_set) {
  free(s->outputs);
  s->channel_count = 0;
  s->counts_set = 0;
 }
}
DP_FN_POSTAMBLE


DP_FN_PREAMBLE(deinterleave,work) {
 uint32_t i,j;
 if (s->counts_set && s->outs_set) {
  for (i=0;i<b->runlength;i++) {
   for (j=0;j<s->channel_count;j++) {
    dp_vec_t *oj = s->outputs[j];
    if (oj) {
     oj->v[i] = b->in_v->v[s->channel_count*i+j];
    }
   }
  }
 }
}
DP_FN_POSTAMBLE

void dp_deinterleave_set_output(dp_base_t *b, uint8_t pos, dp_vec_t *out) {
 DP_IFSUBVALID(b,deinterleave) {
  if (pos<s->channel_count) {
   s->outputs[pos] = out; 
   s->outs_set = dp_true;
  } else {
   fprintf(stderr,"-warn- (%s) - could not set output, pos: %d < channel count %d\n",b->name,pos,s->channel_count);
  }
 }
}

void dp_deinterleave_set_num_channels(dp_base_t *b, uint8_t ccount) {
 uint8_t i;
 DP_IFSUBVALID(b,deinterleave) {
  s->channel_count = ccount;
  s->outputs = malloc(sizeof(dp_vec_t *) * ccount);
  if (s->outputs) {
   for (i=0;i<ccount;i++) { s->outputs[i] = NULL; };
   s->counts_set = dp_true;
  }
 }
}


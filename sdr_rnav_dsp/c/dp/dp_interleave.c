
#include "dp_interleave.h"
#include <stdio.h>

DP_SUB_CREATOR_IMPL(interleave)

DP_FN_PREAMBLE(interleave,init) {
 s->channel_count = 0;
 s->counts_set = dp_false;
 s->ins_set = dp_false;
 b->complex_ok = dp_false;
 b->sub_work = &dp_interleave_work;
 b->sub_deinit = &dp_interleave_deinit;
}
DP_FN_POSTAMBLE


DP_FN_PREAMBLE(interleave,deinit) {
 if (s->inputs && s->ins_set) {
  free(s->inputs);
  s->ins_set = dp_false;
  s->counts_set = dp_false;
  s->channel_count = 0;
 }
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(interleave,work) {
 uint32_t i,j;
 if (s->counts_set && s->ins_set) {
  for (i=0;i<b->runlength;i++) {
   for (j=0;j<s->channel_count;j++) {
    dp_vec_t *ij = s->inputs[j];
    if (ij) {
     b->out_v->v[s->channel_count*i+j] = ij->v[i];
    }
   }
  }
 }
}
DP_FN_POSTAMBLE

void dp_interleave_set_input(dp_base_t *b, uint8_t pos, dp_vec_t *in) {
 DP_IFSUBVALID(b,interleave) {
  if (pos<s->channel_count) {
   s->inputs[pos] = in; 
   s->ins_set = dp_true;
  } else {
   fprintf(stderr,"-cw- (%s) - could not set input, pos: %d is > channel count %d\n",b->name,pos,s->channel_count);
  }
 } 
}

void dp_interleave_set_num_channels(dp_base_t *b, uint8_t ccount) {
 uint32_t i;
 DP_IFSUBVALID(b,interleave) {
  s->channel_count = ccount;
  s->inputs = malloc(sizeof(dp_vec_t *) * ccount);
  if (s->inputs) {
   for (i=0;i<ccount;i++) { s->inputs[i] = NULL; }
   s->counts_set = dp_true;
  }
 }
}


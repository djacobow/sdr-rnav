
#include "dp_base.h"
#include "dp_base_internals.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

dp_base_t *dp_base_create() {
 dp_base_t *b = malloc(sizeof(dp_base_t));
 if (b) {
  memset(b,0,sizeof(dp_base_t));
  b->base_valid = 1;
  b->subt = dp_subt_null;
  b->sub_work    = &dp_nullfn;
  b->sub_prerun  = &dp_nullfn;
  b->sub_postrun = &dp_nullfn;
  b->sub_deinit  = &dp_nullfn;
 }
 return b;
}

void dp_base_create_from(dp_base_t *b) {
 if (b) {
  memset(b,0,sizeof(dp_base_t));
  b->base_valid = 1;
  b->subt = dp_subt_null;
  b->sub_work    = &dp_nullfn;
  b->sub_prerun  = &dp_nullfn;
  b->sub_postrun = &dp_nullfn;
  b->sub_deinit  = &dp_nullfn;
 }
}

void
dp_base_deinit(dp_base_t *b) {
 if (b) {
  if (b->base_valid) {
   if (b->sub_valid) {
    b->sub_deinit(b);
    free(b->sub);
   }
  }
  free(b->name);
 }
}

void dp_destroy(dp_base_t *b) {
 dp_base_deinit(b);
 free(b);
}

void dp_nullfn(dp_base_t *b) { }

void dp_set_in(dp_base_t *b, dp_vec_t *i) { 
 if (DP_VALID(b) && DP_VEC_VALID(i)) {
  b->in_v = i; 
 }
}

void dp_set_inl(dp_base_t *b, dp_vec_t *i, uint32_t il) { 
 if (DP_VALID(b) && DP_VEC_VALID(i)) {
  b->in_v = i; 
  b->runlength = il;
  assert(il <= dp_vec_getlen(i));
  b->length_valid = dp_true;
 }
}

void dp_set_inlt(dp_base_t *b, dp_vec_t *i, uint32_t il, dp_bool_t x) {
 if (DP_VALID(b) && DP_VEC_VALID(i)) {
  b->in_v = i;
  b->runlength = il;
  b->length_valid = dp_true;
  assert(il * (x ? 2 : 1) <= dp_vec_getlen(i));
  b->is_complex = x;
 }
}

void dp_set_out(dp_base_t *b, dp_vec_t *i) { 
 if (DP_VALID(b) && DP_VEC_VALID(i)) {
  b->out_v = i; 
 }
}

void dp_run(dp_base_t *b, uint32_t rmask) {
 if (DP_VALID(b) && (b->group & rmask)) {
  b->sub_work(b); 
 }
}

void dp_prerun(dp_base_t *b) {
 if (b->base_valid && b->sub_valid) {
  b->sub_prerun(b);
 }
}

void dp_postrun(dp_base_t *b) {
 if (b->base_valid && b->sub_valid) {
  b->sub_postrun(b);
 }
}

void dp_set_runlen(dp_base_t *b,uint32_t l) {
 if (DP_VALID(b)) {
  b->runlength = l;
  b->length_valid = 1;
 }
}

void dp_set_complex(dp_base_t *b, dp_bool_t i) {
 if (DP_VALID(b)) {
  if (i) {
   if (b->complex_ok) {
    b->is_complex = i;
   } else {
    fprintf(stderr,"-cw- (%s) block does not handle complex\n",
 		   b->name);
   }
  }
 }
}

void dp_set_group(dp_base_t *b, uint32_t group) {
 if (DP_VALID(b)) { 
  b->group = group;
 }
}

char *dp_get_name(dp_base_t *b) {
 return b->name;
}

void dp_set_name(dp_base_t *b, const char *in) {
 if (DP_VALID(b)) {
  if (b->name) {
   free(b->name);
  };
  b->name = malloc(strlen(in)+1);
  if (b->name) {
   strcpy(b->name,in);
  }
 }
}

void dp_debug_valid(dp_base_t *b, dp_subt_t t) {
 if (!b) {
  fprintf(stderr,"-ce- %s:%d null pointer\n",__FILE__,__LINE__);
 } else if (!b->base_valid) {
  fprintf(stderr,"-ce- %s:%d (%s) block not marked valid\n",__FILE__,__LINE__,b->name);
 } else if (!b->sub) {
  fprintf(stderr,"-ce- %s:%d (%s) null sub pointer\n",__FILE__,__LINE__,b->name);
 } else if (!b->sub_valid) {
  fprintf(stderr,"-ce- %s:%d (%s) sub pointer not marked valid \n",__FILE__,__LINE__,b->name);
 } else if (b->subt != t) {
  fprintf(stderr,"-ce- %s:%d (%s) sub type not equal\n",__FILE__,__LINE__,b->name);
 }
}


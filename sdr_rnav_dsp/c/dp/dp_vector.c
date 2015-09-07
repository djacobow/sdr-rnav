#include "dp_vector.h"
#include <stdio.h>

dp_vec_t *dp_vec_create(uint32_t l) {
 dp_vec_t *v = malloc(sizeof(dp_vec_t));
 dp_vec_init(v,l);
 return v;
}

void dp_vec_init(dp_vec_t *vec, uint32_t l) {
 if (vec) {
  vec->valid = 0;
  vec->v = 0;
  vec->len = 0;
  vec->v = malloc(sizeof(dp_int_t) * l);
  if (vec->v) {
   vec->valid = 1;
   vec->len = l;
  }
 }
}

void dp_vec_deinit(dp_vec_t *v) {
 if (v) {
  if (v->valid) {
   free(v->v);
    v->len = 0;
    v->valid = 0;
  }
 }
}

void dp_vec_destroy(dp_vec_t *v) {
 dp_vec_deinit(v);
 free(v);
}

uint32_t dp_vec_getlen(dp_vec_t *v) {
 return v->len;
}

void dp_vec_resize(dp_vec_t *v, uint32_t l) {
 if (v->valid) {
  dp_int_t *nv = realloc(v->v,sizeof(dp_int_t) * l);
  if (nv) {
   v->v = nv;
   v->len = l;
  }
 } else {
  dp_vec_init(v,l);
 }
}

void dp_vec_show(dp_vec_t *v) {
 dp_vec_show_n(v,v->len);
}

void dp_vec_show_n(dp_vec_t *v, uint32_t l) {
 uint32_t i;
 if (DP_VEC_VALID(v)) { 
  if (l > v->len) { l = v->len; }
  for (i=0;i<l;i++) {
   printf("%d, ", v->v[i]);
  }
  printf("\n");
 }
}


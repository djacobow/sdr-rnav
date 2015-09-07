
#ifndef __DP_VEC_H
#define __DP_VEC_H

#include <stdlib.h>
#include <string.h>
#include "dp_types.h"

typedef struct dp_vec_t {
 dp_int_t *v;
 uint32_t len;
 uint8_t  valid;
} dp_vec_t;

dp_vec_t *dp_vec_create(uint32_t l);
void dp_vec_init(dp_vec_t *v, uint32_t l);
void dp_vec_destroy(dp_vec_t *v);
void dp_vec_deinit(dp_vec_t *v);
void dp_vec_resize(dp_vec_t *v,uint32_t l);

void dp_vec_show(dp_vec_t *v);
void dp_vec_show_n(dp_vec_t *v, uint32_t l);
uint32_t dp_vec_getlen(dp_vec_t *v);

#define DP_VEC_VALID(x) (x->valid && x->len)

#ifdef DEBUG

#define dp_vec_get_at(x,i)   (x->valid ? i < x->len ? x->v[i] : -999 : -999)
#define dp_vec_set_at(x,i,r) { if (x->valid && (i < x->len)) { x->v[i] = r; } }

#else

#define dp_vec_get_at(x,i)   ( x->v[i])
#define dp_vec_set_at(x,i,r) { x->v[i] = r; }

#endif

#endif

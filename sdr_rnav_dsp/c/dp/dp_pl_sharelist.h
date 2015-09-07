#ifndef __DP_PL_SHARELIST_H
#define __DP_PL_SHARELIST_H

#include "dp_base.h"

/* This modules represents a lightweight set of routines that XS 
 * can discover to let perl access variables in C and vice versa.
 *
 * I'm using it because doing the reverse with get_sv seems to 
 * cause problems I don't understand.
 *
 * This is probably not the best approach.
 */

typedef enum dp_pl_share_t_t {
 dp_pl_share_t_t_u32,
 dp_pl_share_t_t_i32,
 dp_pl_share_t_t_d,
 dp_pl_share_t_t_vp,
 dp_pl_share_t_t_fp,
 dp_pl_share_t_t_cp,
 dp_pl_share_t_t_u32p,
 _dp_pl_share_t_t_invalid
} dp_pl_share_t_t;

typedef union dp_pl_share_v_t {
 uint32_t u32;
 int32_t  i32;
 double   d;
 void    *vp;
 float   *fp;
 char    *cp;
 uint32_t *u32p;
} dp_pl_share_v_t;

typedef struct dp_pl_share_t {
 dp_pl_share_v_t v;
 char *n;
 dp_pl_share_t_t t;
} dp_pl_share_t;

typedef struct dp_pl_sharelist_t {
 dp_pl_share_t **list;
 dp_bool_t valid;
 uint32_t size;
 uint32_t count;
} dp_pl_sharelist_t;


dp_pl_sharelist_t *dp_pl_sharelist_create(uint32_t s);
void dp_pl_sharelist_init(dp_pl_sharelist_t *b, uint32_t s);
void dp_pl_sharelist_deinit(dp_pl_sharelist_t *b);
void dp_pl_sharelist_destroy(dp_pl_sharelist_t *b);
void dp_pl_sharelist_destroy_elems(dp_pl_sharelist_t *b);

#define DP_PL_SHARELIST_ADD_DECL(tl,ts) \
int dp_pl_sharelist_add_ ## ts(dp_pl_sharelist_t *b, char *n, tl v)

#define DP_PL_SHARELIST_SET_DECL(tl,ts) \
void dp_pl_sharelist_set_ ## ts(dp_pl_sharelist_t *b, int i, tl v)

#define DP_PL_SHARELIST_GET_DECL(tl,ts) \
tl dp_pl_sharelist_get_ ## ts(dp_pl_sharelist_t *b, int i)

#define DP_PL_SHARELIST_GET_DEREF_DECL(tl,ts) \
tl dp_pl_sharelist_get_deref_ ## ts(dp_pl_sharelist_t *b, int i)

#define DP_PL_ALLOC_TYPE_DECL(tl) \
tl *dp_pl_alloc_ ## tl()

#define DP_PL_ALLOC_TYPE_IMPL(tl) \
tl *dp_pl_alloc_ ## tl() { \
 tl *x = malloc(sizeof(tl)); \
 if (!x) { \
  fprintf(stderr,"-ce- could not alloc requested tl."); \
 } \
 return x; \
} 

#define DP_PL_SHARELIST_ADD_IMPL(tl, ts) \
int dp_pl_sharelist_add_ ## ts(dp_pl_sharelist_t *b, char *n, tl v) { \
 int cidx = b->count; \
 char *tn; \
 if (cidx < b->size) { \
  dp_pl_share_t *ni; \
  ni = malloc(sizeof(dp_pl_share_t)); \
  if (ni) { \
   b->list[cidx] = ni; \
   ni->v.ts = v; \
   ni->t = dp_pl_share_t_t_ ## ts; \
   tn = malloc(strlen(n)+1);  \
   strcpy(tn,n); \
   ni->n = tn; \
   b->count++; \
  } else { \
   cidx = -1; \
  } \
 } else { \
  cidx = -1; \
 }; \
 return cidx; \
}

#define DP_PL_SHARELIST_SET_IMPL(tl,ts) \
void \
dp_pl_sharelist_set_ ## ts(dp_pl_sharelist_t *b, int i, tl v) { \
 b->list[i]->v.ts = v; \
 b->list[i]->t = dp_pl_share_t_t_ ## ts; \
}

#define DP_PL_SHARELIST_GET_IMPL(tl,ts) \
tl dp_pl_sharelist_get_ ## ts(dp_pl_sharelist_t *b, int i) { \
 if (b && b->valid && (i>=0) && (i < b->count) && \
    (b->list[i]->t == dp_pl_share_t_t_ ## ts)) { \
  return b->list[i]->v.ts; \
 } \
 return 0; \
} 

#define DP_PL_SHARELIST_GET_DEREF_IMPL(tl,ts) \
tl dp_pl_sharelist_get_deref_ ## ts(dp_pl_sharelist_t *b, int i) { \
 if (b && b->valid && (i>=0) && (i < b->count) && \
    (b->list[i]->t == dp_pl_share_t_t_ ## ts)) { \
  return *(b->list[i]->v.ts); \
 } \
 return 0; \
} 

DP_PL_SHARELIST_ADD_DECL(uint32_t,  u32);
DP_PL_SHARELIST_ADD_DECL(int32_t,   i32);
DP_PL_SHARELIST_ADD_DECL(double,    d);
DP_PL_SHARELIST_ADD_DECL(void *,    vp);
DP_PL_SHARELIST_ADD_DECL(float*,    fp);
DP_PL_SHARELIST_ADD_DECL(char*,     cp);
DP_PL_SHARELIST_ADD_DECL(uint32_t *,u32p);

DP_PL_SHARELIST_SET_DECL(uint32_t, u32);
DP_PL_SHARELIST_SET_DECL(int32_t,  i32);
DP_PL_SHARELIST_SET_DECL(double,   d);
DP_PL_SHARELIST_SET_DECL(void *,   vp);
DP_PL_SHARELIST_SET_DECL(float*,   fp);
DP_PL_SHARELIST_SET_DECL(char*,    cp);
DP_PL_SHARELIST_SET_DECL(uint32_t *,u32p);

DP_PL_SHARELIST_GET_DECL(uint32_t, u32);
DP_PL_SHARELIST_GET_DECL(int32_t,  i32);
DP_PL_SHARELIST_GET_DECL(double,   d);
DP_PL_SHARELIST_GET_DECL(void *,   vp);
DP_PL_SHARELIST_GET_DECL(float*,   fp);
DP_PL_SHARELIST_GET_DECL(char *,   cp);
DP_PL_SHARELIST_GET_DECL(uint32_t *,u32p);

DP_PL_SHARELIST_GET_DEREF_DECL(float, fp);
DP_PL_SHARELIST_GET_DEREF_DECL(char , cp);
DP_PL_SHARELIST_GET_DEREF_DECL(uint32_t, u32p);

int dp_pl_sharelist_get_idx(dp_pl_sharelist_t *b, char *n);

void dp_pl_sharelist_debug(dp_pl_sharelist_t *b);

DP_PL_ALLOC_TYPE_DECL(float);
DP_PL_ALLOC_TYPE_DECL(uint32_t);
DP_PL_ALLOC_TYPE_DECL(int32_t);
DP_PL_ALLOC_TYPE_DECL(char);
char *dp_pl_alloc_str(uint32_t l);

#endif


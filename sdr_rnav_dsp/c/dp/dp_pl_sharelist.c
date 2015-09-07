
#include "dp_pl_sharelist.h"
#include <stdio.h>

DP_PL_ALLOC_TYPE_IMPL(float)
DP_PL_ALLOC_TYPE_IMPL(uint32_t)
DP_PL_ALLOC_TYPE_IMPL(int32_t)
DP_PL_ALLOC_TYPE_IMPL(char)

char *dp_pl_alloc_str(uint32_t l) {
 char *x = malloc(sizeof(char) * l);
 return x;
}

dp_pl_sharelist_t *dp_pl_sharelist_create(uint32_t s) {
 dp_pl_sharelist_t *b;
 b = malloc(sizeof(dp_pl_sharelist_t));
 if (b) {
  dp_pl_sharelist_init(b,s);
 }
 return b;
}

void dp_pl_sharelist_init(dp_pl_sharelist_t *b, uint32_t s) {
 if (b) {
  b->valid = 1;
  b->count = 0;
  b->list  = malloc(sizeof(dp_pl_share_v_t) * s);
  memset(b->list,0,sizeof(dp_pl_share_v_t) * s);
  if (b->list) {
   b->size = s;
  }
 }
}

void dp_pl_sharelist_deinit(dp_pl_sharelist_t *b) {
 if (b && b->valid && b->size) {
  free(b->list);
  b->size = 0;
 }
}

void dp_pl_sharelist_destroy(dp_pl_sharelist_t *b) {
 if (b) {
  dp_pl_sharelist_deinit(b);
 }
 free(b);
}

void dp_pl_sharelist_destroy_elems(dp_pl_sharelist_t *b) {
 uint32_t i;
 if (b && b->valid) {
  for (i=0;i<b->count;i++) {
   free(b->list[i]->n);
   free(b->list[i]);
   b->list[i] = 0;
  }
 } 
}



DP_PL_SHARELIST_ADD_IMPL(uint32_t , u32)
DP_PL_SHARELIST_ADD_IMPL(int32_t , i32)
DP_PL_SHARELIST_ADD_IMPL(double , d)
DP_PL_SHARELIST_ADD_IMPL(void *, vp)
DP_PL_SHARELIST_ADD_IMPL(float*, fp)
DP_PL_SHARELIST_ADD_IMPL(char*, cp)
DP_PL_SHARELIST_ADD_IMPL(uint32_t*,u32p)

int dp_pl_sharelist_get_idx(dp_pl_sharelist_t *b, char *n) {
 int i;
 if (b && b->valid && b->count) {
  for (i=0;i<b->count;i++) {
   if (!strcmp(b->list[i]->n,n)) {
    return i;
   } 
  }
 }
 return -1;
}

DP_PL_SHARELIST_SET_IMPL(uint32_t,u32)
DP_PL_SHARELIST_SET_IMPL(int32_t,i32)
DP_PL_SHARELIST_SET_IMPL(double,d)
DP_PL_SHARELIST_SET_IMPL(void *,vp)
DP_PL_SHARELIST_SET_IMPL(float *,fp)
DP_PL_SHARELIST_SET_IMPL(char*,cp)
DP_PL_SHARELIST_SET_IMPL(uint32_t*,u32p)


DP_PL_SHARELIST_GET_IMPL(void *,vp)
DP_PL_SHARELIST_GET_IMPL(uint32_t, u32)
DP_PL_SHARELIST_GET_IMPL(int32_t, i32)
DP_PL_SHARELIST_GET_IMPL(double, d)
DP_PL_SHARELIST_GET_IMPL(float *, fp)
DP_PL_SHARELIST_GET_IMPL(char *, cp)
DP_PL_SHARELIST_GET_IMPL(uint32_t*, u32p)

DP_PL_SHARELIST_GET_DEREF_IMPL(float,fp)
DP_PL_SHARELIST_GET_DEREF_IMPL(char,cp)
DP_PL_SHARELIST_GET_DEREF_IMPL(uint32_t,u32p)


void dp_pl_sharelist_debug(dp_pl_sharelist_t *b) {
 if (b) {
  if (b->valid) {
   int i;
   fprintf(stderr,"-cd- sharelist size  %d\n",b->size);
   fprintf(stderr,"-cd- sharelist count %d\n",b->count);
   for (i=0;i<b->count;i++) {
    switch (b->list[i]->t) {
     case  dp_pl_share_t_t_cp : {
      fprintf(stderr,"-cd- i %d cp 0x%8.8x (%c) name %s\n",i,(unsigned int)b->list[i]->v.cp,*(b->list[i]->v.cp),b->list[i]->n);
      break;
     }
     case  dp_pl_share_t_t_u32p : {
      fprintf(stderr,"-cd- i %d u32p 0x%8.8x (%d) name %s\n",i,(unsigned int)b->list[i]->v.fp,*(b->list[i]->v.u32p),b->list[i]->n);
      break;
     }
     case  dp_pl_share_t_t_fp : {
      fprintf(stderr,"-cd- i %d fp 0x%8.8x (%f) name %s\n",i,(unsigned int)b->list[i]->v.fp,*(b->list[i]->v.fp),b->list[i]->n);
      break;
     }
     case  dp_pl_share_t_t_vp : {
      fprintf(stderr,"-cd- i %d vp 0x%8.8x name %s\n",i,(unsigned int)b->list[i]->v.vp,b->list[i]->n);
      break;
     }
     case  dp_pl_share_t_t_i32 : {
      fprintf(stderr,"-cd- i %d i32 %d name %s\n",i,b->list[i]->v.i32,b->list[i]->n);
      break;
     }
     case  dp_pl_share_t_t_u32 : {
      fprintf(stderr,"-cd- i %d u32 %d name %s\n",i,b->list[i]->v.u32,b->list[i]->n);
      break;
     }
     case  dp_pl_share_t_t_d: {
      fprintf(stderr,"-cd- i %d d %f name %s\n",i,b->list[i]->v.d,b->list[i]->n);
      break;
     }
     default:  {
      fprintf(stderr,"-cd- i %d <invalid_type> name %s\n",i,b->list[i]->n);
      break;
     }
    }
   } 
  } else {
   fprintf(stderr,"-cd- sharelist not marked valid\n");
  }
 } else {
  fprintf(stderr,"-cd- sharelist null\n");
 };
}


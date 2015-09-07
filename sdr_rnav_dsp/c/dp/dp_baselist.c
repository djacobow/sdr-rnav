
#include "dp_base.h"
#include "dp_baselist.h"
#include <stdio.h>

dp_baselist_t *dp_baselist_create(uint32_t s) {
 dp_baselist_t *b;
 b = malloc(sizeof(dp_baselist_t));
 if (b) {
  dp_baselist_init(b,s);
 } 
 return b;
}

void dp_baselist_init(dp_baselist_t *b, uint32_t s) {
 if (b) {
  b->valid = 1;
  b->count = 0;
  b->list = malloc(sizeof(dp_base_t *) * s);
  if (b->list) {
   b->size = s;
  }
 }
}

void dp_baselist_deinit(dp_baselist_t *bl) {
 if (bl) {
  if (bl->valid) {
   if (bl->size) {
     free(bl->list);
   }
  }
 }
}

void dp_baselist_destroy(dp_baselist_t *bl) {
 if (bl) {
  dp_baselist_deinit(bl);
 }
 free(bl);
}

void dp_baselist_destroy_elems(dp_baselist_t *bl) {
 uint32_t i;
 if (bl && bl->valid) {
  for (i=0;i<bl->count;i++) {
   dp_destroy(bl->list[i]);
  }
 }
}

void dp_baselist_add(dp_baselist_t *bl, dp_base_t *b) {
 if (bl && b) {
  if (bl->valid && b->base_valid) {
   if (bl->count < bl->size) {
    bl->list[bl->count++] = b;
   }
  }
 }
}


void dp_baselist_prerun(dp_baselist_t *bl) {
 uint32_t i;
 if (bl && bl->valid) {
  for (i=0;i<bl->count;i++) {
   /* fprintf(stderr,"-ci- pre-running object %s\n",dp_get_name(bl->list[i])); */
   dp_prerun(bl->list[i]);
  }
 }
}

void dp_baselist_postrun(dp_baselist_t *bl) {
 uint32_t i;
 if (bl && bl->valid) {
  for (i=0;i<bl->count;i++) {
   /* printf("-ci- post-running object %s\n",dp_get_name(bl->list[i])); */
   dp_postrun(bl->list[i]);
  }
 }
}

void dp_baselist_run(dp_baselist_t *bl, uint32_t rmask) {
 uint32_t i;
 /* printf("-cd dp_baselist_run\n"); */
 if (bl && bl->valid) {
  for (i=0;i<bl->count;i++) {
   /* printf("-ci- running object %s\n",dp_get_name(bl->list[i])); */
   dp_run(bl->list[i],rmask);
  }
 }
}


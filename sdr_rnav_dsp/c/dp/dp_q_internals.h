
#include "dp_base.h"

/* Implements a simple array based queue. In a macro to make it type-flexible. */

/* forgive me for the abuse of the preprocessor I am about to 
 * commit ... */

#define DP_QUEUE_INSTANTIATE_HEADER(name,type) \
 \
typedef struct dp_q_ ## name ## _t { \
 type **contents; \
 unsigned int front; \
 unsigned int rear; \
 unsigned int max_size; \
} dp_q_ ## name ## _t; \
 \
dp_bool_t  \
dp_q_ ## name ## _init(dp_q_ ## name ##_t *b, unsigned int msize);\
\
void \
dp_q_ ## name ## _deinit(dp_q_ ## name ##_t *b); \
\
dp_bool_t \
dp_q_ ## name ## _insert(dp_q_ ## name ##_t *b, type *v); \
\
void \
dp_q_ ## name ## _insert_force(dp_q_ ## name ##_t *b, type *v); \
\
dp_bool_t \
dp_q_ ## name ## _pop(dp_q_ ## name ## _t *b, type *vl); \
\
dp_bool_t \
dp_q_ ## name ## _empty(dp_q_ ## name ## _t *b); \
\
dp_bool_t \
dp_q_ ## name ## _full(dp_q_ ## name ## _t  *b);\
\
unsigned int \
dp_q_ ## name ## _fullness(dp_q_ ## name ## _t *b)








#define DP_QUEUE_INSTANTIATE_IMPL(name,type) \
\
dp_bool_t  \
dp_q_ ## name ## _init(dp_q_ ## name ##_t *b, unsigned int msize) { \
 uint32_t i; \
 dp_bool_t rv = true; \
 memset(b,0,sizeof(dp_q_ ## name ## _t)); \
 b->max_size = msize; \
 b->contents = malloc(sizeof(type *) * b->max_size); \
 if (b->contents) { \
  for (i=0;i<b->max_size;i++) { \
   b->contents[i] = malloc(sizeof(type)); \
   if (!b->contents[i]) { rv = false; } \
  } \
 } else { \
  rv = false; \
 } \
 return rv; \
} \
\
\
\
void \
dp_q_ ## name ## _deinit(dp_q_ ## name ##_t *b) { \
 uint32_t i; \
 for (i=0;i<b->max_size;i++) { \
  free(b->contents[i]); \
 } \
} \
\
\
\
dp_bool_t \
dp_q_ ## name ## _insert(dp_q_ ## name ##_t *b, type *v) { \
 int fullness = (b->front - b->rear); \
 if (fullness < 0) { fullness += b->max_size; } \
 if (fullness < (b->max_size-1)) { \
  memcpy(b->contents[b->front++],v,sizeof(type)); \
  if (b->front == b->max_size) { b->front = 0; } \
  return true; \
 } else {\
  return false; \
 } \
} \
\
\
\
void \
dp_q_ ## name ## _insert_force(dp_q_ ## name ##_t *b, type *v) { \
 int fullness = (b->front - b->rear); \
 if (fullness < 0) { fullness += b->max_size; } \
 if (fullness == (b->max_size-1)) { \
  b->rear++; \
  if (b->rear == b->max_size) { b->rear = 0; } \
 } \
 memcpy(b->contents[b->front++],v,sizeof(type)); \
 if (b->front == b->max_size) { b->front = 0; } \
} \
\
\
\
dp_bool_t \
dp_q_ ## name ## _pop(dp_q_ ## name ## _t *b, type *v) { \
 if (b->rear == b->front) { \
  return false; \
 } else { \
  memcpy(v,b->contents[b->rear++],sizeof(type)); \
  if (b->rear == b->max_size) { b->rear = 0; } \
  return true; \
 } \
} \
\
\
\
dp_bool_t \
dp_q_ ## name ## _full(dp_q_ ## name ## _t *b) { \
 int fullness = b->front - b->rear; \
 if (fullness < 0) { fullness += b->max_size; } \
 return (fullness == (b->max_size -1)); \
} \
\
\
\
dp_bool_t \
dp_q_ ## name ## _empty(dp_q_ ## name ## _t *b) { \
 return (b->front == b->rear); \
} \
\
\
\
unsigned int \
dp_q_ ## name ## _fullness(dp_q_ ## name ## _t *b) { \
 int fullness = b->front - b->rear; \
 if (fullness < 0) { fullness += b->max_size; } \
 return fullness; \
}



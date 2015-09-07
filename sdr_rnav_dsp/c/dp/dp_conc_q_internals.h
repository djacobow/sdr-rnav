
#include "dp_base.h"

/* adds concurrency protection on top of a queue */

/* more macro madness ... */

/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

// This implements a thread-safe queue class on a generic object, but with 
// a few wrinkles of semantics. In particular, there is a try_pop_all, which
// sucks everything out of the queue and returns only the last item. There
// is also push_max which will push an object onto the queue, but if there
// are more than max_size objects after the push, a few will be popped off
// to keep the total queue length only max_size;
*/

#include <pthread.h>

#define DP_CONC_QUEUE_INSTANTIATE_HEADER(name,type) \
 \
typedef struct dp_conc_q_ ## name ## _t { \
 dp_q_ ## name ## _t the_queue; \
 pthread_mutex_t     the_mutex; \
 pthread_cond_t      the_condition_variable; \
} dp_conc_q_ ## name ## _t; \
\
dp_bool_t \
dp_conc_q_ ## name ## _init(dp_conc_q_ ## name ## _t *b, unsigned int msize); \
\
void \
dp_conc_q_ ## name ## _deinit(dp_conc_q_ ## name ## _t *b); \
\
dp_bool_t \
dp_conc_q_ ## name ## _push(dp_conc_q_ ## name ## _t *b, type *v); \
\
void \
dp_conc_q_ ## name ## _push_max(dp_conc_q_ ## name ## _t *b, type *v); \
\
dp_bool_t \
dp_conc_q_ ## name ## _empty(dp_conc_q_ ## name ## _t *b); \
\
dp_bool_t \
dp_conc_q_ ## name ## _try_pop_all(dp_conc_q_ ## name ## _t *b, type *v); \
\
dp_bool_t \
dp_conc_q_ ## name ## _try_pop(dp_conc_q_ ## name ## _t *b, type *v); \
\
void \
dp_conc_q_ ## name ## _wait_and_pop(dp_conc_q_ ## name ## _t *b, type *v) \



#define DP_CONC_QUEUE_INSTANTIATE_IMPL(name, type) \
\
dp_bool_t \
dp_conc_q_ ## name ## _init(dp_conc_q_ ## name ## _t *b, unsigned int msize) { \
 pthread_mutex_init(&b->the_mutex,0); \
 pthread_cond_init(&b->the_condition_variable,0); \
 return dp_q_ ## name ## _init(&b->the_queue, msize); \
} \
\
void \
dp_conc_q_ ## name ## _deinit(dp_conc_q_ ## name ## _t *b) { \
 pthread_mutex_destroy(&b->the_mutex); \
 pthread_cond_destroy(&b->the_condition_variable); \
 dp_q_ ## name ## _deinit(&b->the_queue);\
}\
\
dp_bool_t \
dp_conc_q_ ## name ## _push(dp_conc_q_ ## name ##_t *b, type *v) {\
 dp_bool_t rv;\
 pthread_mutex_lock(&b->the_mutex);\
 rv = dp_q_ ## name ## _insert(&b->the_queue, v);\
 pthread_mutex_unlock(&b->the_mutex);\
 pthread_cond_signal(&b->the_condition_variable);\
 return rv; \
}\
\
void \
dp_conc_q_ ## name ## _push_max(dp_conc_q_ ## name ## _t *b, type *v) {\
 pthread_mutex_lock(&b->the_mutex);\
 dp_q_ ## name ## _insert_force(&b->the_queue, v);\
 pthread_mutex_unlock(&b->the_mutex);\
 pthread_cond_signal(&b->the_condition_variable);\
}\
\
dp_bool_t \
dp_conc_q_ ## name ## empty(dp_conc_q_ ## name ## _t *b) {\
 dp_bool_t rv;\
 pthread_mutex_lock(&b->the_mutex);\
 rv = dp_q_ ## name ## _empty(&b->the_queue);\
 pthread_mutex_unlock(&b->the_mutex);\
 return rv;\
}\
\
dp_bool_t \
dp_conc_q_ ## name ## _try_pop(dp_conc_q_ ## name ## _t *b, type *v) { \
 dp_bool_t rv = false; \
 pthread_mutex_lock(&b->the_mutex); \
 if (dp_q_ ## name ## _empty(&b->the_queue)) { \
  rv = false; \
 } else { \
  rv = true; \
  dp_q_ ## name ## _pop(&b->the_queue,v); \
 } \
 pthread_mutex_unlock(&b->the_mutex); \
 return rv; \
} \
\
dp_bool_t \
dp_conc_q_ ## name ## _try_pop_all(dp_conc_q_ ## name ## _t *b, type *v) {\
 dp_bool_t rv = false;\
 pthread_mutex_lock(&b->the_mutex);\
 if (dp_q_ ## name ## _empty(&b->the_queue)) {\
  rv = false;\
 } else {\
  rv = true;\
  while (dp_q_ ## name ## _fullness(&b->the_queue)) {\
   dp_q_ ## name ## _pop(&b->the_queue,v);\
  }\
 }\
 pthread_mutex_unlock(&b->the_mutex);\
 return rv;\
}\
\
dp_bool_t \
dp_conc_q_ ## name ## _pop(dp_conc_q_ ## name ## _t *b, type *v) {\
 dp_bool_t rv = false;\
 pthread_mutex_lock(&b->the_mutex);\
 if(dp_q_ ## name ## _empty(&b->the_queue)) {\
  rv = false;\
 } else {\
  rv = dp_q_ ## name ## _pop(&b->the_queue, v);\
 }\
 pthread_mutex_unlock(&b->the_mutex);\
 return rv;\
}\
\
void \
dp_conc_q_ ## name ## _wait_and_pop(dp_conc_q_ ## name ## _t *b, type *v) {\
 pthread_mutex_lock(&b->the_mutex);\
 while(dp_q_ ## name ## _empty(&b->the_queue)) {\
  pthread_cond_wait(&b->the_condition_variable, &b->the_mutex);\
 }\
 dp_q_ ## name ## _pop(&b->the_queue, v);\
 pthread_mutex_unlock(&b->the_mutex);\
}\




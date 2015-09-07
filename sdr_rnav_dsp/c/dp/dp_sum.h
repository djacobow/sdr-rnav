/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_SUM_H
#define DP_SUM_H

#include "dp_base_internals.h"

#define MAX_ADDENDS (16)

typedef struct dp_sum_str_t {
 dp_vec_t *v;
 float scale_f;
 float scale_i;
} dp_sum_str_t;

typedef struct dp_sum_sub_t {
 dp_sum_str_t *sum_vecs;
 uint32_t vecs_ct;
 uint32_t max_vecs;
} dp_sum_sub_t;

DP_SUB_CREATOR_DECL(sum);
DP_FN_DECL(sum,work);
DP_FN_DECL(sum,init);
DP_FN_DECL(sum,deinit);

dp_bool_t dp_sum_add_addend(dp_base_t *b, dp_vec_t *f, float scale);

#endif



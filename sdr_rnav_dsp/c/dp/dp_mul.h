/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_MULTIPLY_H 
#define DP_MULTIPLY_H 

#include "dp_base_internals.h"

#define MAX_FACTORS (16)

typedef struct dp_mul_sub_t {
 dp_vec_t **mul_vecs;
 uint32_t vecs_ct;
 uint32_t max_vecs;
} dp_mul_sub_t;

DP_SUB_CREATOR_DECL(mul);
DP_FN_DECL(mul,work);
DP_FN_DECL(mul,init);
DP_FN_DECL(mul,deinit);

dp_bool_t dp_mul_add_factor(dp_base_t *b, dp_vec_t *f);

#endif




/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_ECOMPARE_H
#define DP_ECOMPARE_H

/* Compare the energy in two signals */

#include "dp_base_internals.h"

DP_SUB_CREATOR_DECL(ecompare);
DP_FN_DECL(ecompare,init);
DP_FN_DECL(ecompare,work);

void dp_ecompare_set_in_2(dp_base_t *b, const dp_vec_t *i);
void dp_ecompare_set_ins(dp_base_t *b, dp_vec_t *i1, dp_vec_t *i2);
void dp_ecompare_set_out(dp_base_t *b, float *o);

typedef struct dp_ecompare_sub_t {
  const dp_vec_t *in2;
  float *outvar;
} dp_ecompare_sub_t;


#endif


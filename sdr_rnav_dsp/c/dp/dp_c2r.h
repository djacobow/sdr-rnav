
/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef _DP_C2R_H
#define _DP_C2R_H

#include "dp_base_internals.h"

typedef struct dp_c2r_sub_t {
 int _dummy; /* for C90 pedantic */
} dp_c2r_sub_t;

uint32_t dp_SquareRoot(uint32_t a_nInput);
dp_int_t dp_comp16tomag16(dp_int_t r, dp_int_t i);

DP_SUB_CREATOR_DECL(c2r);
DP_FN_DECL(c2r,init);
DP_FN_DECL(c2r,work);

#endif



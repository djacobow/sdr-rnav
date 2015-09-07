
/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef _DP_DECIMATE_H
#define _DP_DECIMATE_H

#include "dp_base_internals.h"

typedef struct dp_decimate_sub_t {
 uint32_t decim;
} dp_decimate_sub_t;

DP_SUB_CREATOR_DECL(decimate);
DP_FN_DECL(decimate,work);
DP_FN_DECL(decimate,decimate);
DP_FN_DECL(decimate,cdecimate);
DP_FN_DECL(decimate,init);

DP_SUB_GENERIC_SETTER_DECL(decimate,decim,uint32_t);

#endif

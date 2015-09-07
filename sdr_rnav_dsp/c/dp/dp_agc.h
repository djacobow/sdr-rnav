/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Fall 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_AGC_H
#define DP_AGC_H

#include "dp_base_internals.h"

typedef struct dp_agc_sub_t {
 float   time_constant;
 float   avg_mag_tconst_frames;
 float   desired_mag;
} dp_agc_sub_t;

DP_SUB_CREATOR_DECL(agc);
DP_SUB_GENERIC_SETTER_DECL(agc,time_constant,float);
DP_SUB_GENERIC_SETTER_DECL(agc,desired_mag,float);
DP_FN_DECL(agc,work);
DP_FN_DECL(agc,init);

#endif




/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_FINDPEAKS_H 
#define DP_FINDPEAKS_H

#define DP_MAX_PTS (32)

#include "dp_base_internals.h"

typedef struct peak_pt_t {
 int32_t bin; /* the frequency bin */
 float   db;  /* the dB above the *average* of all sample */
 int16_t abs; /* the actual value of the sample */
} peak_pt_t;

typedef struct peak_pts_t {
 peak_pt_t **points;
 uint32_t maxpts;
 uint32_t actpts;
 uint32_t length;
 int32_t average;
 uint32_t iteration;
} peak_pts_t;

void peak_pts_t_init(peak_pts_t *p);
void peak_pts_t_deinit(peak_pts_t *p);

DP_SUB_CREATOR_DECL(findpeaks);
DP_FN_DECL(findpeaks,work);
DP_FN_DECL(findpeaks,prerun);
DP_FN_DECL(findpeaks,init);
DP_SUB_GENERIC_SETTER_DECL(findpeaks,out,peak_pts_t*);
DP_SUB_GENERIC_SETTER_DECL(findpeaks,threshold,int); /* argument in dB */

typedef struct dp_findpeaks_sub_t {
  float ln10;
  peak_pts_t *out;
  uint32_t iter;
  int32_t threshold;
} dp_findpeaks_sub_t;

#endif


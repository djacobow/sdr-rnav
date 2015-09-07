
/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef __DP_FIR_C_H
#define __DP_FIR_C_H

#include "dp_base_internals.h"

DP_SUB_CREATOR_DECL(fir);
DP_FN_DECL(fir,init);
DP_FN_DECL(fir,work);
DP_FN_DECL(fir,prerun);
DP_FN_DECL(fir,deinit);
DP_FN_DECL(fir,prep_filter);
DP_FN_DECL(fir,work_single);
DP_FN_DECL(fir,work_interleaved);
void dp_fir_set_filter(dp_base_t *, dp_int_t *, uint32_t, dp_bool_t);

typedef struct dp_fir_sub_t {
 dp_vec_t taps;
 dp_vec_t hbuffer1;
 dp_vec_t hbuffer2;
 uint32_t tlen;
 const int16_t *filter_inptr;
 uint32_t filter_inlen;
 dp_bool_t filter_is_set;
 dp_bool_t filter_is_ready;
 dp_bool_t hb_is_set;
 dp_bool_t filter_is_complex;
} dp_fir_sub_t;

#endif



/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz
*/


/*
   Not complete and not completely working.

   See notes in the associated source file.

*/

#ifndef __DP_FFTFILT_C_H
#define __DP_FFTFILT_C_H

#include "dp_base_internals.h"
#include "kiss_fft.h"
#include "kiss_fftr.h"


DP_SUB_CREATOR_DECL(fft_fir);
DP_FN_DECL(fft_fir,init);
DP_FN_DECL(fft_fir,work);
DP_FN_DECL(fft_fir,prerun);
DP_FN_DECL(fft_fir,deinit);
DP_FN_DECL(fft_fir,prepare_filter);
DP_FN_DECL(fft_fir,work_real);
DP_FN_DECL(fft_fir,work_complex);

void dp_fft_fir_set_filter(dp_base_t *, dp_int_t *, uint32_t, dp_bool_t);

typedef struct dp_fft_fir_sub_t {
  const int16_t *filter_inptr;
  uint32_t filter_inlen;
  dp_bool_t filter_is_complex;
  dp_bool_t filter_is_set;
  dp_bool_t filter_is_ready;
  dp_bool_t fft_ready;
  uint32_t fft_len;
  kiss_fft_cpx *fdom_buf;
  /* this buffer could be saved but it helped with debugging: */
  kiss_fft_cpx *fdom_conv_buf; 

  kiss_fft_cfg fft_fcfg_c;
  kiss_fft_cfg fft_icfg_c;
  kiss_fft_cpx *input_buf_c;
  kiss_fft_cpx *output_buf_c;
  kiss_fft_cpx *last_output_buf_c;

  kiss_fftr_cfg    fft_fcfg_r;
  kiss_fftr_cfg    fft_icfg_r;
  kiss_fft_scalar *input_buf_r;
  kiss_fft_scalar *output_buf_r;
  kiss_fft_scalar *last_output_buf_r;

  kiss_fft_cpx *filt_buf;
  int curr_shift;
} dp_fft_fir_sub_t;

uint32_t powerTwoGreater(uint32_t in);

#endif



// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz


//////////////////////////////////////////////////

/*
   Not complete and working.

   See notes in the associated source file.

*/

#ifndef __FFTFILT_C_H
#define __FFTFILT_C_H

#include "block_base.h"
#include "kiss_fft.h"
#include "kiss_fftr.h"

namespace djdsp {

class fft_filt_c : public block_base_c {

 public:
  fft_filt_c();
  // void design_lpf(uint32_t sr, float trans_start, float trans_end);
  // void design_bpf(uint32_t sr, float t1_start, float t1_end, float t2_start, float t2_end);
  void set_filter(const int16_t *nt, uint32_t ntlen, sig_cpx_t s);
  void pre_run();
  void work();
  ~fft_filt_c();

 private:
  void prepare_filter();
  void work_complex();
  void work_real();
  void make_unready();
  bool filter_is_set;
  bool filter_is_ready;
  const int16_t *filter_inptr;
  uint32_t filter_inlen;
  bool filter_is_complex;
  bool fft_ready;
  uint32_t fft_len;
  kiss_fft_cpx *fdom_buf;
  // this buffer could be saved but it helped with debugging:
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
};

uint32_t powerTwoGreater(uint32_t in);

} // namespace

#endif


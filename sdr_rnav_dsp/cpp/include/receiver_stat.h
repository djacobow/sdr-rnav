
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef __RCVR_STAT_H
#define __RCVR_STAT_H

#include "rtl_help.h"
#include "cmix.h"

// Everything you'd want to know about the current state 
// of the VOR receiver, maybe a bit more. :-)
typedef struct receiver_stat_t {
 float       strength_ratio;
 float       strength_ratio_lpf;
 bool        have_carrier;
 float       ref30_period;
 float       var30_period;
 float       phase_diff;
 float       phase_diff_lpf;
 uint32_t    nf_sr;
 float       xcorr_delay;
 float       xcorr_delay_lpf;
 uint32_t    block_time_ms;
 djdsp::rtl_help_c *radio;
 djdsp::cmixer_c   *mixer;
 uint32_t    buffer_count;
 bool        use_mixer;
 bool        run_fft;
 int32_t     mixer_lo_freq;
 uint32_t    tune_freq;
} receiver_stat_t;

#endif


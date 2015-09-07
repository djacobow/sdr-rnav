
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef __RCVR_STAT_H
#define __RCVR_STAT_H

#include "dp_radio2832.h"
#include "dp_cmix.h"

/* Everything you'd want to know about the current state 
   of the VOR receiver, maybe a bit more. :-) */
typedef struct receiver_stat_t {
 float       strength_ratio;     /* dB -- ish */
 float       strength_ratio_lpf; /* dB -- ish */
 dp_bool_t   have_carrier;   /* bool */
 float       ref30_period;   /* Hz */
 float       var30_period;   /* Hz */
 float       phase_diff;     /* s  */
 float       phase_diff_lpf; /* s  */
 uint32_t    nf_sr;          /* Hz */
 uint32_t    block_time_ms;  /* ms */
 dp_base_t  *radio;
 dp_base_t  *mixer;
 uint32_t    buffer_count;
 dp_bool_t   use_mixer;
 dp_bool_t   run_fft;
 int32_t     mixer_lo_freq; /* Hz */
 uint32_t    tune_freq;  /* Hz */
} receiver_stat_t;

 /* alternative phase measure.
    maybe we'll put these back in one day ... */
 /* float       xcorr_delay; */
 /* float       xcorr_delay_lpf; */

#endif


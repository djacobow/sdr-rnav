
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef __TEST_MAIN_H
#define __TEST_MAIN_H

#include "dp_base.h"

extern int             _main_max_cap_seconds;
extern unsigned int    _main_freq;
extern dp_bool_t       _main_use_mixer;
extern dp_bool_t       _main_perform_fft;
extern float           _main_mixer_lo_freq;
extern float           _main_radial_calibrate;
extern float           _main_min_snr;
extern int             _main_port;
extern volatile int    _main_decode_has_begun;
extern volatile int    _main_ctrl_has_begun;
extern float           _main_fft_pk_thresh;

#define MAX_FN_LEN (512)

extern char _main_odir[MAX_FN_LEN];
extern char _main_infile[MAX_FN_LEN];

#endif



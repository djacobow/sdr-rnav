
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef __TEST_MAIN_H
#define __TEST_MAIN_H

/*
typedef enum sig_path_ctrl_t {
 sp_decode_mix,
 sp_decode_nomix,
 sp_fftonly,
 sp_invalid,
} sig_path_ctrl_t;
*/


extern int             _main_max_cap_seconds;
extern unsigned int    _main_freq;
extern bool            _main_use_mixer;
extern bool            _main_perform_fft;
extern float           _main_mixer_lo_freq;
extern float           _main_radial_calibrate;
extern float           _main_min_snr;
extern int             _main_port;

#ifdef __WIN32
const std::string idir = "..\\test_input_files\\";
const std::string odir = "..\\test_output_files\\";
#else
const std::string idir = "../test_input_files/";
const std::string odir = "../test_output_files/";
#endif

extern std::string _main_infile;

#endif



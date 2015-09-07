#ifndef _TOP_LEVEL_H
#define _TOP_LEVEL_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <iostream>
#include <ctime>
#include <string.h>


#ifdef __linux
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#endif

#include "rtl_help.h"
#include "dsp_chain.h"
#include "my_console.h"

extern const std::string idir;
extern const std::string odir;

// These defaults will all correctly decode the OAK file listed below,
// and they can all be overriddent from the command line with getArgs()
extern int             _main_max_cap_seconds;
extern unsigned int    _main_freq;
extern bool            _main_use_mixer;
extern bool            _main_perform_fft;
extern float           _main_mixer_lo_freq;
extern float           _main_radial_calibrate;
extern std::string     _main_infile;
extern float           _main_min_snr;
extern int             _main_port;

extern djdsp::rtl_help_c radio;
extern receiver_stat_t lrstat;
extern djdsp::peak_pts_t pts;
extern std::string id_instr;
extern bool have_status;
extern bool have_fft;

#endif


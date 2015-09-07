#ifndef _TOP_LEVEL_H
#define _TOP_LEVEL_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>


#ifdef __linux
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#endif

#include "dp_radio2832.h"
#include "dsp_chain.h"
/* #include "my_console.h" */

#define MAX_INFILE_LEN (1024)

extern const char *idir, *odir;

/* These defaults will all correctly decode the OAK file listed below,
   and they can all be overriddent from the command line with getArgs()
*/
extern int             _main_max_cap_seconds;
extern unsigned int    _main_freq;
extern dp_bool_t       _main_use_mixer;
extern dp_bool_t       _main_perform_fft;
extern float           _main_mixer_lo_freq;
extern float           _main_radial_calibrate;
extern char            _main_infile[MAX_INFILE_LEN];
extern char            _main_odir[MAX_INFILE_LEN];
extern float           _main_min_snr;
extern float           _main_fft_pk_thresh;
extern int             _main_ctrl_has_begun;
extern int             _main_decode_has_begun;

extern dp_base_t *radio;
extern receiver_stat_t lrstat;
extern peak_pts_t pts;
extern char id_instr[80];
extern dp_bool_t have_status;
extern dp_bool_t have_fft;

#endif


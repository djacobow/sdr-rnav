
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef _DSP_CHAIN_H
#define _DSP_CHAIN_H

#include "dp_base.h"
#include "dp_baselist.h"

#include "my_qs.h"

/* This is the parameters for the simple trailing average IIR
// LPF that smooths the radial reporting */
#ifndef PH_LPF_FACTOR
#define PH_LPF_FACTOR (0.1)
#endif

void *dsp_thread_fn(void *f);

extern dp_conc_q_rstat_t _dsp_chain_rstat_queue;
extern dp_conc_q_char_t  _dsp_chain_id_text_queue;
extern dp_conc_q_peaks_t _dsp_chain_peaks_queue;
extern dp_bool_t         _dsp_chain_all_done;

#endif


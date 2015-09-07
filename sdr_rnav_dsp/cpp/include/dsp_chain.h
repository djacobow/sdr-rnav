
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef _DSP_CHAIN_H
#define _DSP_CHAIN_H

#include "block_base.h"
#include "conc_str_queue.h"
#include "conc_rstat_queue.h"
#include "conc_peaks_queue.h"

// This is the parameters for the simple trailing average IIR
// LPF that smooths the radial reporting
#ifndef PH_LPF_FACTOR
#define PH_LPF_FACTOR (0.1)
#endif

typedef std::vector<djdsp::block_base_c *> blockps_t;

extern "C" {
void *dsp_thread_fn(void *f);
}

extern concurrent_rstat_queue_c _dsp_chain_rstat_queue;
extern concurrent_str_queue_c   _dsp_chain_id_text_queue;
extern concurrent_peaks_queue_c _dsp_chain_peaks_queue;
extern bool                     _dsp_chain_all_done;
#endif


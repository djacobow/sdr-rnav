
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <sstream>
#include <ctime>
#include <string.h>
#include <assert.h>

// These are all DSP blocks; subclassed from block_base_c
#include "wav_w.h"
#include "wav_r.h"
#include "fir.h"
#include "fft_filt.h"
#include "deinterleave.h"
#include "interleave.h"
#include "scale.h"
#include "multiply.h"
#include "makesig.h"
#include "sum.h"
#include "autofilter.h"

using namespace djdsp;

typedef std::vector<block_base_c *> blockps_t;

const int R_BUF_LEN = 8192;

int main(int argc, char *argv[]) {

 dvec_t gen_buffer(R_BUF_LEN);
 dvec_t filt_buffer(R_BUF_LEN);
 dvec_t write_buffer(2*R_BUF_LEN);

 blockps_t blocks;

 makesig_c fgen;
 fgen.set_out(&gen_buffer);
 fgen.set_sample_rate(44100);
 fgen.set_sine(4000,30000,0,0);
 fgen.set_runlen(R_BUF_LEN);
 fgen.set_group(1);
 blocks.push_back(&fgen);

 // setup and initialize low pass for post-mixer
 /*
__firdes__start__yaml__
---
 - name: lp1k
   type: fir
   subtype: bandpass
   sample_rate: 44100 
   bands:
    - start: 0
      end: 900 
    - start: 1100 
      end: 3900 
    - start: 4100 
      end: 22050 
   mags:
    - 0
    - 1
    - 0
   ripple:
    - -70dB
    - 1dB
    - -70dB
__firdes__end__yaml__
*/

#define USE_FFT (1)

#if USE_FFT 
 fft_filt_c filt;
#else
 fir_c filt;
#endif
 filt.set_out(&filt_buffer);
 filt.set_in(&gen_buffer,R_BUF_LEN);
 filt.set_filter(lp1k_coeffs,lp1k_len,s_real);
 filt.set_group(1);
 blocks.push_back(&filt);

 interleave_c ei;
 ei.set_num_channels(2);
 ei.set_input(1,&filt_buffer);
 ei.set_input(0,&gen_buffer);
 ei.set_out(&write_buffer);
 ei.set_runlen(R_BUF_LEN);
 ei.set_group(1);
 blocks.push_back(&ei);

 wav_w_c writer;
 writer.set_name("writer");
 writer.set_bits_per_sample(16);
 writer.set_num_channels(2);
 writer.set_sample_rate(44100);
 writer.set_in(&write_buffer,R_BUF_LEN);
 writer.set_file("results.wav");
 writer.set_group(1);
 blocks.push_back(&writer);

 for (uint32_t i=0;i<blocks.size();i++) {
  blocks[i]->pre_run();
 }

 bool done = false;
 uint32_t bnum = 0;
 while (!done) {
  for (uint32_t i=0;i<blocks.size();i++) {
   blocks[i]->run(1);
  }
  done = bnum >= 100;
  bnum++;

  // for (uint32_t k=0;k<50;k++) { d3c(k,gen_buffer[k],filt_buffer[k]); }
  // std::cout << std::endl << std::endl << std::endl;
 }

 for (uint32_t i=0;i<blocks.size();i++) {
  blocks[i]->post_run();
 }

 return 0;
};


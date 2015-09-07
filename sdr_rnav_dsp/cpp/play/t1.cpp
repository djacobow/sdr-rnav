
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
#include "fft_filt.h"
#include "fir.h"
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


 blockps_t blocks;

 dvec_t read_buffer(R_BUF_LEN*2);
 dvec_t read_buffer_l(R_BUF_LEN);
 dvec_t read_buffer_l2(R_BUF_LEN);
 dvec_t read_buffer_r(R_BUF_LEN);
 dvec_t boop_buffer(R_BUF_LEN);
 dvec_t write_buffer(R_BUF_LEN*2);

 wav_r_c reader;
 reader.set_name("reader");
 reader.set_out(&read_buffer);
 reader.set_runlen(R_BUF_LEN);
 reader.set_file("Studio60-O-Holy-Night-NOLA.wav");
 reader.set_group(1);
 blocks.push_back(&reader);

 deinterleave_c di;
 di.set_num_channels(2);
 di.set_in(&read_buffer,R_BUF_LEN);
 di.set_output(0,&read_buffer_l);
 di.set_output(1,&read_buffer_r);
 di.set_group(1);
 blocks.push_back(&di);

 scale_c scale;
 scale.set_out(&read_buffer_l2);
 scale.set_in(&read_buffer_l, R_BUF_LEN);
 scale.set_group(1);
 scale.set_scale_f(1.0);
 blocks.push_back(&scale);

 makesig_c fgen;
 fgen.set_out(&boop_buffer);
 fgen.set_sample_rate(44100);
 fgen.set_sine(466.164 /2,3000,0,0);
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

 fft_filt_c filt;
 // fir_c filt;
 filt.set_out(&boop_buffer);
 filt.set_in(&read_buffer_l,R_BUF_LEN,s_real);
 filt.set_group(1);
 filt.set_filter(lp1k_coeffs,lp1k_len,s_real);
 blocks.push_back(&filt);

 sum_c sum;
 sum.set_name("summer");
 sum.set_out(&read_buffer_l2);
 sum.set_complex(s_real);
 sum.add_addend(&read_buffer_l,0.00);
 sum.add_addend(&boop_buffer,1);
 sum.set_runlen(R_BUF_LEN);
 sum.set_group(1);
 blocks.push_back(&sum);

 interleave_c ei;
 ei.set_num_channels(2);
 ei.set_input(1,&read_buffer_r);
 ei.set_input(0,&read_buffer_l2);
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
  uint32_t le = reader.lastElems();
  if (le == 0) { done = true; };
  bnum++;
 }

 for (uint32_t i=0;i<blocks.size();i++) {
  blocks[i]->post_run();
 }

 return 0;
};


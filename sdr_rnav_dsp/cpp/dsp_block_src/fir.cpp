// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz

// A straightforward and quasi-canonical but still probably compiler-
// unrollable implementation of fixed point FIR filters. Should work 
// fine as long as the number of taps in the filter is less than the 
// size of the buffer. Note that I do the filter in two pieces, a complicated
// bit and a simple bit. I could have done it in one piece, but I think
// this makes the less complicated bit (which is fine for most of the
// samples) more accessible to the compiler for optimization. Not that
// I've run with optimization turned on or looked at any assembly output
// yet...

#include "fir.h"
#include <stdio.h>
#include <string.h>
#include "helpers.h"

namespace djdsp {

fir_c::fir_c() {
 filter_is_set = false;
 filter_is_ready = false;
 hb_is_set = false;
 is_complex = false;
 filter_is_complex = false;
 complex_ok = true;
};

fir_c::~fir_c() {
 if (filter_is_ready) {
  // cleanup stuff
 };
};

void
fir_c::set_filter(const int16_t *nt, uint32_t ntlen, sig_cpx_t s) {
 filter_is_complex = s == s_complex;
 filter_inptr = nt;
 filter_inlen = ntlen;
 tlen = ntlen;
 filter_is_set = true;
 filter_is_ready = false;
};

void fir_c::pre_run() {
 if (filter_is_set && !filter_is_ready) {
  prepare_filter();
 }
};

void 
fir_c::prepare_filter() {

 if (filter_is_set) {
  if (filter_is_complex) {
   taps.resize(2*filter_inlen);
   for (uint32_t i=0;i<(filter_inlen-1);i++) {
    taps[2*i]   = filter_inptr[2*i];
    taps[2*i+1] = filter_inptr[2*i+1];
   }
   taps[2*(filter_inlen-1)]   = filter_inptr[2*(filter_inlen-1)];
   taps[2*(filter_inlen-1)+1] = filter_inptr[2*(filter_inlen-1)+1];
  } else {
   taps.resize(filter_inlen);
   for (uint32_t i=0;i<(filter_inlen-1);i++) {
    taps[i] = filter_inptr[i];
   }
   taps[filter_inlen-1] = filter_inptr[filter_inlen-1];
  }

  filter_is_ready = true;
  hb_is_set = false;
 }
};

void 
fir_c::work() {
 if (!filter_is_ready && filter_is_set) {
  prepare_filter();
 } else if (filter_is_ready) {
  if (is_complex) {
   work_interleaved();
  } else {
   work_single();
  }
 }
};

void
fir_c::work_single() {
 uint32_t bcount = l;
 uint32_t tcount = tlen;
 uint32_t hcount = bcount + tcount;

 if (!hb_is_set) {
  hbuffer1.resize(hcount);
  for (uint32_t i=0;i<hcount;i++) { hbuffer1[i] = 0; };
  hb_is_set = true;
 };

 for (uint32_t i=0;i<tcount;i++) {
  hbuffer1[i] = hbuffer1[bcount+i];
 };
 for (uint32_t i=tcount;i<hcount;i++) {
  hbuffer1[i] = (*in)[i-tcount];
 }

 int32_t acc = 0;
 for (uint32_t i=0;i<bcount;i++) {
  acc = 1 << 14;
  for (uint32_t k=0;k<tcount;k++) {
   uint32_t hidx = tcount + i - k;
   acc += taps[k] * hbuffer1[hidx];
  }
  acc = SATURATE30(acc);
  (*out)[i] = (int16_t)(acc >> 15);
 };
};



void
fir_c::work_interleaved() {
 uint32_t bcount = l;
 uint32_t tcount = tlen;
 uint32_t hcount = bcount + tcount;

 if (!hb_is_set) {
  hbuffer1.resize(hcount);
  hbuffer2.resize(hcount);
  for (uint32_t i=0;i<hcount;i++) { 
   hbuffer1[i] = 0; 
   hbuffer2[i] = 0; 
  };
  hb_is_set = true;
 };

 for (uint32_t i=0;i<tcount;i++) {
  hbuffer1[i] = hbuffer1[bcount+i];
  hbuffer2[i] = hbuffer2[bcount+i];
 };
 for (uint32_t i=tcount;i<hcount;i++) {
  hbuffer1[i] = (*in)[2*(i-tcount)];
  hbuffer2[i] = (*in)[2*(i-tcount)+1];
 }

 int32_t acc1 = 0;
 int32_t acc2 = 0;
 for (uint32_t i=0;i<bcount;i++) {
  acc1 = 1 << 14;
  acc2 = 1 << 14;
  if (filter_is_complex) {
   for (uint32_t k=0;k<tcount;k++) {
    uint32_t hidx = tcount + i - k;
    acc1 += taps[2*k]   * hbuffer1[hidx] - taps[2*k+1] * hbuffer2[hidx];
    acc2 += taps[2*k+1] * hbuffer2[hidx] + taps[2*k]   * hbuffer1[hidx];
   }
  } else {
   for (uint32_t k=0;k<tcount;k++) {
    uint32_t hidx = tcount + i - k;
    acc1 += taps[k] * hbuffer1[hidx];
    acc2 += taps[k] * hbuffer2[hidx];
   }
  }
  acc1 = SATURATE30(acc1);
  acc2 = SATURATE30(acc2);
  (*out)[(2*i)+0] = (int16_t)(acc1 >> 15);
  (*out)[(2*i)+1] = (int16_t)(acc2 >> 15);
 };
};



} // namespace

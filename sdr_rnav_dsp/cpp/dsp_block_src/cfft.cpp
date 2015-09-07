
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <iostream>

#include "cfft.h"
#include "kiss_fft.h"

namespace djdsp {

// Implements a complex mixer class. Takes an IQ signal and performs
// a complex multiply with a sin/cos complex signal generated to match
// a given LO frequency provided. The LO frequency can be positive or 
// negative to slide the spectrum up or down. To slide up, provide a 
// negative value. 

cfft_c::cfft_c() {
 ready = false;
 buffs_allocd = false;
 // optimization removed for now; the fft can share 
 // buffers with the in/out except that I've decided
 // to implement averaging/smoothing, which makes that
 // not work, at least for the output buffer.
 // int iq_size = sizeof(kiss_fft_cpx);
 // int twosize = 2 * sizeof(myint_t);
 // need_buffers = twosize != iq_size;
 need_buffers = 1;
 averaging = 0;
 complex_ok = true;
};

cfft_c::~cfft_c() {
 if (buffs_allocd) {
  delete ibuff;
  delete obuff;
  kiss_fft_cleanup();
 }
};

void
cfft_c::set_averaging(float f) {
 if (f<0) { f = 0; };
 if (f>1) { f = 1; };

 int mul_old = f * 256.0;
 if (mul_old > 255) {
  mul_old = 255;
 }
 averaging = mul_old;
};

void
cfft_c::pre_run() {
 if (!is_complex) {
  std::cout << "-error- (" << name << ") real fft not supported yet"
	    << std::endl;
  exit -1;
 }

 if (need_buffers && !buffs_allocd) {
  std::cout << "-info- cfft_c (" << name << ") allocating buffer" << std::endl;
  ibuff = new kiss_fft_cpx[l];
  obuff = new kiss_fft_cpx[l];
  buffs_allocd = true;
 }

 if (!ready) {
  cfg = kiss_fft_alloc(l,0,0,0);
  ready = true;
 }
};

// Some optimization work needs to be done. This function 
// can work directly from the input and output buffers for the 
// object, but there are two downsides. 1) we'd still need
// another buffer to do smoothing and 2) the output order of the 
// fft is not convenient for visualization. It has 0 Hz at index 0
// rather than centered int the list. The (-1)^i flipping below
// fixes that, but requires another buffer.
void 
cfft_c::work() {
  if (need_buffers && !buffs_allocd) {
   pre_run();
  }

  if (need_buffers) {
   for (uint32_t i=0;i<l;i++) {
    ibuff[i].i = (*in)[2*i]    * ((i%2) ? -1 : 1);
    ibuff[i].r = (*in)[2*i+1]  * ((i%2) ? -1 : 1);
   } 
  } else {
   ibuff = (kiss_fft_cpx *)(void *)((*in).data());
   obuff = (kiss_fft_cpx *)(void *)((*out).data());
  }

  // std::cout << "-info- cfft_c (" << name << ") work" << std::endl;
  kiss_fft(cfg,ibuff, obuff);

  if (need_buffers) {
   for (uint32_t i=0;i<l;i++) {
    int32_t av_r, av_i;
    av_r = (255 - averaging) * obuff[i].r + averaging * (*out)[2*i];
    av_i = (255 - averaging) * obuff[i].i + averaging * (*out)[2*i+1];
    (*out)[2*i]   = av_r >> 8;
    (*out)[2*i+1] = av_i >> 8;
   }
  }
};

} // namespace

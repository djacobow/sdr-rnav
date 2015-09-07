

// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

// Implementaton of an IIR filter class. 
//
// So far (7/15/2013) unused, and completely untested. Almost certainly 
// horribly broken. The problem with IIRs from a fixed-point perspective
// is that they have constants whose values vary from small fractions to 
// large numbers. To maintain precision, floating point, or something
// approximating it is necessary.
//
// When I get around
// to it, I may convert some FIRs into IIRs for performance.

#include "iir.h"
#include <stdio.h>
#include <string.h>
#include "helpers.h"
#include <math.h>

namespace djdsp {

iir_c::iir_c() {
 is_set = false;
 hb_is_set = false;
};

iir_c::~iir_c() {
 if (is_set) {
 }
};

void fp_to_int16_and_shift(double fin, int16_t *iout, int8_t *shout) {

 int8_t rsbits = 0;
 bool neg = (fin < 0.0);
 fin = fabs(fin);
 if ((fin <= (1.0/32768.0)) && (fin >= (-1.0/32768.0))) {
  *iout = 0;
  *shout = 0; 
 } else if (fin > 32768.0) {
  while (fin > 32768.0) {
   fin /= 2.0;
   rsbits++;
  }
  *iout = (int16_t)fin;
  if (neg) { *iout = -*iout; };
  *shout = rsbits;
 } else {
  while (fin < 16384.0) {
   fin *= 2;
   rsbits--;
  }
  *iout = (int16_t)fin;
  if (neg) { *iout = -*iout; };
  *shout = rsbits;
 };
};


void
iir_c::set_filter(const double *nt, uint32_t ntlen) {
 x_taps.resize(ntlen);
 y_taps.resize(ntlen);
 x_shft.resize(ntlen);
 y_shft.resize(ntlen);

 tlen = ntlen;
 for (uint32_t i=0;i<ntlen;i++) {
  double xti_fp = nt[2*i];
  double yti_fp = nt[2*i+1];

  int16_t xti_int;
  int8_t xti_shft;
  fp_to_int16_and_shift(xti_fp,&xti_int,&xti_shft);
  x_taps[i] = xti_int;
  x_shft[i] = xti_shft;

  int16_t yti_int;
  int8_t yti_shft;
  fp_to_int16_and_shift(yti_fp,&yti_int,&yti_shft);
  y_taps[i] = yti_int;
  y_shft[i] = yti_shft;

  printf("x[%d] orig: %.6f int %d rshift %d\n",i,xti_fp,xti_int,xti_shft);
  printf("y[%d] orig: %.6f int %d rshift %d\n",i,yti_fp,yti_int,yti_shft);


 }
 is_set = true;
 hb_is_set = false;
};

void
iir_c::work() {
 uint32_t bcount = l;
 uint32_t tcount = tlen;
 uint32_t hcount = bcount + tcount;

 if (!hb_is_set) {
  xbuffer.resize(hcount);
  ybuffer.resize(hcount);
  for (uint32_t i=0;i<hcount;i++) { 
   xbuffer[i] = 0; 
   ybuffer[i] = 0; 
  };
  hb_is_set = true;
 }

 for (uint32_t i=0;i<tcount;i++) {
  xbuffer[i] = xbuffer[bcount+i];
  ybuffer[i] = ybuffer[bcount+i];
 }
 for (uint32_t i=tcount;i<hcount;i++) {
  xbuffer[i] = (*in)[i-tcount];
 }

 int32_t acc = 0;
 for (uint32_t i=0;i<bcount;i++) {
  acc = 0;
  for (uint32_t k=0;k<tcount;k++) {
   uint32_t xidx = tcount + i - k;
   acc += (x_taps[k] * xbuffer[xidx]) >> x_shft[k];
   acc += (y_taps[k] * ybuffer[xidx]) >> y_shft[k];
  }
  acc = SATURATE30(acc);
  int16_t res = (int16_t)(acc >> 15);
  (*out)[i] = res;
  ybuffer[i+tcount] = res;
 };
}

} // namespace

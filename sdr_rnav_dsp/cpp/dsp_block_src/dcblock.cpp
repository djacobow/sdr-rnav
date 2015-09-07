

// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz


#include "dcblock.h"
#include "helpers.h"
#include <iostream>

namespace djdsp {

// Implements a simple HPF DC-blocker. Actually, the current implementation
// is not right, but works well enough. I "DC block" by subtracting an offset
// equal to a moveing average of the signal over the buffer (and last few 
// buffers) provided.
//
// This gets the job done, roughly.


// from http://www.dspguru.com/dsp/tricks/fixed-point-dc-blocking-filter-with-noise-shaping
// http://www.dsprelated.com/dspbooks/filters/DC_Blocker.html

dcblock_c::dcblock_c() {
 pole   = 0.99;
 for (int i=0;i<4;i++) { hist[i] = 0; };
 adjust();
};

int16_t
dcblock_c::adjust() {
 A      = 32768.0*(1.0-pole);
 acc    = 0;
 prev_x = 0;
 prev_y = 0;
 s_factor = pole*(float)32767;
 return s_factor;
};

void
dcblock_c::set_pole(float p) {
 if ((p > 0.9) && (p<1)) {
  pole = p;
 } else {
  std::cerr << "-error- pole must be between 0.9 and 1" << std::endl;
  exit(-1);
 }
};


dcblock_c::~dcblock_c() { }

/*
void
dcblock_c::work(int16_t *x, int16_t *y, uint32_t l) {
 for (uint32_t n=0;n<l;n++) {
  int32_t Rpy = SATURATE30(s_factor * prev_y);
  Rpy >>= 15;
  int32_t n_y_32 = (int32_t)x[n] - prev_x + Rpy;
  y[n] = SATURATE16(n_y_32);

  prev_x = x[n];
  prev_y = y[n]; 
 };

}
*/



/*
   // I think this one works, but may have rounding/saturation issues
   // for large inputs. Need to fix.
void
dcblock_c::work(int16_t *x, int16_t *y, uint32_t l) {
 for (uint32_t n=0;n<l;n++) {
   acc   -= prev_x;
   prev_x = (int32_t)x[n]<<15;
   acc   += prev_x;
   acc   -= A*prev_y;
   prev_y = acc>>15;               // quantization happens here
   y[n]   = (int16_t)prev_y;
   // acc has y[n] in upper 17 bits and -e[n] in lower 15 bits
 };
};
*/


/*
 * This is my pathetic DC blocking hack. Just subtract the average 
 * of the block. To make it a bit more sophisticated, it takes out
 * the weighted average of this block and the last three, with 
 * decreasing weight.
 */
void 
dcblock_c::work() {
  int32_t s = 0;
  for (uint32_t n=0;n<l;n++) {
   s += (*in)[n];
  }
  s /= l;
  int32_t adj_amt = (s/2) + (hist[0]/4) + (hist[1]+hist[2])/8;
  for (uint32_t n=0;n<l;n++) {
   (*out)[n] = (*in)[n] - adj_amt;
  }

  // s-= adj_amt;
  hist[3] = hist[2];
  hist[2] = hist[1];
  hist[1] = hist[0];
  hist[0] = s;
};

} // namespace

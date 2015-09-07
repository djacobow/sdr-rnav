// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz

#include "decimate.h"

// This is a very simple decimation. Not moving average, just dropping
// samples. It only works for integer relationships. It's not a resampler.

namespace djdsp {

decimate_c::decimate_c() {
 complex_ok = true;
 is_complex = false;
 decim = 1;
}

decimate_c::~decimate_c() {
}

void 
decimate_c::set_decim(uint32_t d) {
 decim = d; 
}

void 
decimate_c::work() {
  if (is_complex) {
   cdecimate();
  } else {
   decimate();
  }
}



void 
decimate_c::decimate() {
 uint32_t len = l;
 uint32_t max_i = 0;

 for (uint32_t i=0;i<len/decim;i++) {
  uint32_t idx_in  = i*decim;
  (*out)[i] = (*in)[idx_in];
  if (i>max_i) { max_i = i; };
 };

};

// complex version. Same as regular version but manages the I,Q stride
// of the arrays
void 
decimate_c::cdecimate() {
 uint32_t len = l;
 // d2(len, decim);
 for (uint32_t i=0;i<len/decim;i++) {
  uint32_t idx_in  = 2*i*decim;
  uint32_t idx_out = 2*i;

  (*out)[idx_out]   = (*in)[idx_in ];
  (*out)[idx_out+1] = (*in)[idx_in+1];
 };
};

} // namespace

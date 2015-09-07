
#include "interleave.h"

#include <iostream>

namespace djdsp {


interleave_c::interleave_c() {
 channel_count = 0;
 counts_set = false;
 ins_set = false;
}

interleave_c::~interleave_c() {
 //
};


void
interleave_c::work() {
 if (counts_set && ins_set) {
  for (uint32_t i=0;i<l;i++) {
   for (uint8_t j=0;j<channel_count;j++) {
    dvec_t *ij = inputs[j];
    if (ij) {
     (*out)[channel_count*i+j] = (*ij)[i];
    }
   }
  };
 }
};

void interleave_c::set_input(uint8_t pos, dvec_t *in) {
 if (pos<channel_count) {
  inputs[pos] = in; 
  ins_set = true;
 } else {
  std::cout << "-warn- (" << name << ") - could not set input, pos: "
	    << pos << " is less than channel count " << channel_count
	    << std::endl;
 }
};

void interleave_c::set_num_channels(uint8_t ccount) {
 channel_count = ccount;
 inputs.resize(ccount);
 for (uint8_t i=0;i<ccount;i++) { inputs[i] = NULL; };
 counts_set = true;
};

} // namespace

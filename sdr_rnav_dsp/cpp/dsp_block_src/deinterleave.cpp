#include "deinterleave.h"

#include <iostream>


namespace djdsp {


deinterleave_c::deinterleave_c() {
 channel_count = 0;
 counts_set = false;
 outs_set = false;
}

deinterleave_c::~deinterleave_c() {
 //
};


void
deinterleave_c::work() {
 if (counts_set && outs_set) {
  for (uint32_t i=0;i<l;i++) {
   for (uint8_t j=0;j<channel_count;j++) {
    dvec_t *oj = outputs[j];
    if (oj) {
     (*oj)[i] = (*in)[channel_count*i+j];
    }
   }
  };
 }
};

void deinterleave_c::set_output(uint8_t pos, dvec_t *out) {
 if (pos<channel_count) {
  outputs[pos] = out; 
  outs_set = true;
 } else {
  std::cout << "-warn- (" << name << ") - could not set output, pos: "
	    << pos << " is less than channel count " << channel_count
	    << std::endl;
 }
};

void deinterleave_c::set_num_channels(uint8_t ccount) {
 channel_count = ccount;
 outputs.resize(ccount);
 for (uint8_t i=0;i<ccount;i++) { outputs[i] = NULL; };
 counts_set = true;
};

} // namespace

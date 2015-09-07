#include "scale.h"
#include "c2r.h"

#include <cmath>

namespace djdsp {

scale_c::scale_c() {
 is_complex = false;
 scale_set = false;
 complex_ok = true;
}

void
scale_c::set_scale_i(int16_t i, int16_t q) {
 scale_i = i;
 scale_q = q;
 scale_set = true;
};

void
scale_c::set_scale_i(int16_t i) {
 scale_i = i;
 scale_q = 0;
 scale_set = true;
};

void
scale_c::set_scale_f(double f) {
 if (f<-1) { f = -1; };
 if (f>1)  { f = 1;  };
 f *= 32767.0;
 scale_i = f;
 scale_q = 0;
 scale_set = true;
};


void
scale_c::set_scale_f(double fi, double fq) {
 double fm = sqrt(fi*fi+fq*fq);
 if (fm>1) {
  fi /= fm;
  fq /= fm;
 } 
 scale_i = fi * 32767.0;
 scale_q = fq * 32767.0;
 scale_set = true;
};


void
scale_c::work() {
 if (scale_set) {
  if (is_complex) { 
   for (uint32_t i=0;i<l;i++) {
    int32_t ii = (*in)[2*i];
    int32_t iq = (*in)[2*i+1];
    int32_t pi = ii * scale_i - iq * scale_q;
    int32_t pq = ii * scale_q + iq * scale_i;
    (*out)[2*i]   = (SATURATE30(pi)) >> 15; 
    (*out)[2*i+1] = (SATURATE30(pq)) >> 15; 
   }
  } else {
   for (uint32_t i=0;i<l;i++) {
    int32_t p = (*in)[i];
    p *= scale_i;
    (*out)[i] = (SATURATE30(p)) >> 15;
   }
  }
 }
};

} // namespace

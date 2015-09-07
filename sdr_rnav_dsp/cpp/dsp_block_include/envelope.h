
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef _ENVELOPE_H
#define _ENVELOPE_H

#include "block_base.h"

namespace djdsp {

class envelope_c : public block_base_c {

 public:
  envelope_c();
  ~envelope_c();
  void work();
  void set_sample_rate(uint16_t sr);
  void set_attack_s(float a);
  void set_release_s(float r);

 private:
  int16_t floatToFixed(float f);
  int16_t timeToCoeff(float t);
  float attack_time;
  float release_time;
  int16_t g_attack;
  int16_t g_release;
  uint16_t sample_rate;
  // float fenv;
  int32_t env;
};

} // namespace

#endif




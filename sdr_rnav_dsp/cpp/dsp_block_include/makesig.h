
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz

#ifndef MAKESIG_H
#define MAKESIG_H

#include "block_base.h"

namespace djdsp {

typedef enum {
 sig_sine,
 sig_cos,
 sig_square,
 sig_ramp,
 sig_pulse,
 sig_invalid
} makesig_sigtype_t;

class makesig_c : public block_base_c {
 public:
  makesig_c();
  void work();
  void set_sine(float freq /* Hz */, int16_t amplitude, int16_t offset, float ph /* radians */);
  void set_square(float freq, int16_t amplitude, int16_t offset);
  void set_pulse(float freq, int16_t amplitude, int16_t offset, float duty_cycle);
  void set_ramp(float freq, int16_t amplitude, int16_t offset, float up_percent);
  void set_sample_rate(uint32_t sr);
 private:
  void setup();
  makesig_sigtype_t stype;
  int16_t  amplitude;
  int16_t  offset;
  float    phase;
  uint32_t sample_rate;
  float    period_samples;
  uint32_t cpos;
  bool     wave_on_fly;
  float    duty_cycle;
  dvec_t   stored_waveform;
  bool     is_setup;
  float    frequency;
};

} // namespace

#endif



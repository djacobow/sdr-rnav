
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_MAKESIG_H
#define DP_MAKESIG_H

#include "dp_base_internals.h"

typedef enum {
 sig_sine,
 sig_cos,
 sig_square,
 sig_ramp,
 sig_pulse,
 sig_invalid
} makesig_sigtype_t;


DP_SUB_CREATOR_DECL(makesig);
DP_FN_DECL(makesig,init);
DP_FN_DECL(makesig,work);
DP_FN_DECL(makesig,setup);
DP_FN_DECL(makesig,deinit);
DP_SUB_GENERIC_SETTER_DECL(makesig,sample_rate,uint32_t);

void dp_makesig_set_sine(dp_base_t *, float freq /* Hz */, int16_t amplitude, int16_t offset, float ph /* radians */);
void dp_makesig_set_square(dp_base_t *, float freq, int16_t amplitude, int16_t offset);
void dp_makesig_set_pulse(dp_base_t *, float freq, int16_t amplitude, int16_t offset, float duty_cycle);
void dp_makesig_set_ramp(dp_base_t *, float freq, int16_t amplitude, int16_t offset, float up_percent);



typedef struct dp_makesig_sub_t {
  makesig_sigtype_t stype;
  int16_t   amplitude;
  int16_t   offset;
  float     phase;
  uint32_t  sample_rate;
  float     period_samples;
  uint32_t  cpos;
  dp_bool_t wave_on_fly;
  float     duty_cycle;
  dp_vec_t  stored_waveform;
  dp_bool_t is_setup;
  float     frequency;
} dp_makesig_sub_t;

#endif



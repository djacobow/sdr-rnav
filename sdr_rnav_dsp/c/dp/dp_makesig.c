/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz
*/

#include "dp_makesig.h"
#include <math.h>
#include <stdio.h>

#ifndef PI
#define PI (3.14159265358)
#endif

#define CLAMP16I(v) ((v > 32767) ? 32767 : (v < -32767) ? -32767 : v)

const uint32_t max_stored_waveform = 16 * 1024;

DP_SUB_CREATOR_IMPL(makesig)

DP_FN_PREAMBLE(makesig,init) {
 s->stype = sig_invalid;
 s->offset = 0;
 s->amplitude = 0x7fff;
 s->period_samples = 0;
 s->cpos = 0;
 s->sample_rate = 44100;
 s->phase = 0;
 s->is_setup = dp_false;
 b->sub_work = &dp_makesig_work;
 b->sub_deinit = &dp_makesig_deinit;
}
DP_FN_POSTAMBLE


DP_FN_PREAMBLE(makesig,deinit) {
 dp_vec_deinit(&s->stored_waveform);
}
DP_FN_POSTAMBLE


DP_SUB_GENERIC_SETTER_IMPL(makesig,sample_rate,uint32_t)



DP_FN_PREAMBLE(makesig,setup) {
 float fittable_waveforms;
 float wave_length_exact;
 uint32_t wave_length_exact_int;
 uint32_t i;
 int32_t si;
 float sv;
 float cut_point_exact;
 s->period_samples = (float)s->sample_rate / s->frequency;
 fittable_waveforms = (float)max_stored_waveform / s->period_samples;
 s->wave_on_fly = (fittable_waveforms < 1.0);
 if (!s->wave_on_fly) {
  fittable_waveforms = floor(fittable_waveforms);
  wave_length_exact    = fittable_waveforms * s->period_samples;
  wave_length_exact_int  = floor(wave_length_exact+0.5);
  dp_vec_resize(&s->stored_waveform,wave_length_exact_int);
  switch (s->stype) {
   case sig_ramp : {
    cut_point_exact = s->period_samples * s->duty_cycle;
    for (i=0;i<wave_length_exact_int;i++) {
     float cyc_fraction = fmod(i,s->period_samples) + s->phase;
     float sv = (cyc_fraction < cut_point_exact) ?
	     cyc_fraction / cut_point_exact :
	     (s->period_samples - cyc_fraction) / (s->period_samples - cut_point_exact);
     si = sv * s->amplitude + s->offset;
     si = CLAMP16I(si);
     s->stored_waveform.v[i] = si;
    }
    break;
   }
   case sig_pulse : {
    float turn_off_exact = s->duty_cycle * s->period_samples;
    for (i=0;i<wave_length_exact_int;i++) {
     float fraction = fmod(i,s->period_samples) + s->phase;
     int sv = (fraction < turn_off_exact);
     si = sv * s->amplitude + s->offset;
     si = CLAMP16I(si);
     s->stored_waveform.v[i] = si;
    }
    break;
   }
   case sig_sine : {
    for (i=0;i<wave_length_exact_int;i++) {
     float angle = 2 * PI * (float)i / s->period_samples + s->phase;
     while (angle > (2.0*PI))  { angle -= 2.0 * PI; };
     sv = sin(angle);
     si = sv * s->amplitude + s->offset;
     si = CLAMP16I(si);
     s->stored_waveform.v[i] = si;
    }
    break;
   }
   default : {
   }
  }
 }
 s->is_setup = dp_true;
}
DP_FN_POSTAMBLE


void
dp_makesig_set_ramp(dp_base_t *b, float f, int16_t a, int16_t o, float w) {
 DP_IFSUBVALID(b,makesig) {
  s->frequency = f;
  s->amplitude = a;
  s->offset = o;
  s->stype = sig_ramp;
  if (w < 0)      { w = 0; }
  else if (w > 1) { w = 1;};
  s->duty_cycle = w;
  s->is_setup = dp_false;
 } 
}


void
dp_makesig_set_pulse(dp_base_t *b, float f, int16_t a, int16_t o, float w) {
 DP_IFSUBVALID(b,makesig) {
  s->frequency = f;
  s->amplitude = a;
  s->offset = o;
  s->stype = sig_pulse;
  if (w < 0)      { w = 0; }
  else if (w > 1) { w = 1;};
  s->duty_cycle = w;
  s->is_setup = dp_false;
 }
}


void 
dp_makesig_set_square(dp_base_t *b, float f, int16_t a, int16_t o) {
 DP_IFSUBVALID(b,makesig) {
  s->frequency = f;
  s->amplitude = a;
  s->offset = o;
  s->stype = sig_pulse;
  s->duty_cycle = 0.5;
  s->is_setup = dp_false;
 }
}
  
void
dp_makesig_set_sine(dp_base_t *b, float f, int16_t a, int16_t o, float ph) {
 DP_IFSUBVALID(b,makesig) {
  s->frequency = f;
  s->amplitude = a;
  s->offset = o;
  s->phase = ph;
  s->stype = sig_sine;
  s->is_setup = dp_false;
 } 
}


DP_FN_PREAMBLE(makesig,work) {
 uint32_t i;
 if (!s->is_setup) {
  dp_makesig_setup(b);
 }

 if (!s->wave_on_fly) {
  uint32_t p_samps = dp_vec_getlen(&s->stored_waveform);
  for (i=0;i<b->runlength;i++) {
   uint32_t idx = s->cpos % p_samps;
   int32_t si = s->stored_waveform.v[idx];
   b->out_v->v[i] = si;
   s->cpos++;
  }
 } else {
  uint32_t period_samples_int = floor(s->period_samples+0.5);
  float cut_exact = s->period_samples * s->duty_cycle;
  int32_t si;
  fprintf(stderr,"wave on fly\n");
  for (i=0;i<b->runlength;i++) {
   float cyc_fraction = fmod(s->cpos,s->period_samples);
   float sv;
   switch (s->stype) {
    case sig_ramp: {
      sv = (cyc_fraction < cut_exact) ? 
       cyc_fraction / cut_exact :
       (s->period_samples-cyc_fraction) / (s->period_samples - cut_exact);
      break;
    }
    case sig_pulse : {
      sv = (cyc_fraction < cut_exact) ? 1 : 0;
      break;
    }
    case sig_sine : {
     float angle = (2 * PI * (float)(s->cpos % period_samples_int) / (float)period_samples_int) + s->phase;
     sv = sin(angle);
     break;
    }
    default : {
     sv = 0;
    }
   }
   si = s->amplitude * sv + s->offset;
   si = CLAMP16I(si);
   b->out_v->v[i] = si;
   s->cpos++;
  }
 }
}
DP_FN_POSTAMBLE



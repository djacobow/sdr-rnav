// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz

#include "makesig.h"
#include <cmath>
#include <iostream>

#ifndef PI
#define PI (3.14159265358)
#endif

#define CLAMP16I(v) ((v > 32767) ? 32767 : (v < -32767) ? -32767 : v)

namespace djdsp {

const uint32_t max_stored_waveform = 16 * 1024;


makesig_c::makesig_c() {
 stype = sig_invalid;
 offset = 0;
 amplitude = 0x7fff;
 period_samples = 0;
 cpos = 0;
 sample_rate = 44100;
 phase = 0;
 is_setup = false;
};

void
makesig_c::set_sample_rate(uint32_t sr) {
 sample_rate = sr;
};



void 
makesig_c::setup() {
 period_samples = (float)sample_rate / frequency;
 // f1(period_samples);
 float fittable_waveforms = (float)max_stored_waveform / period_samples;
 wave_on_fly = (fittable_waveforms < 1.0);
 if (!wave_on_fly) {
  fittable_waveforms = floor(fittable_waveforms);
  float wave_length_exact    = fittable_waveforms * period_samples;
  uint32_t wave_length_exact_int  = floor(wave_length_exact+0.5);
  // d1(wave_length_exact_int);
  stored_waveform.resize(wave_length_exact_int);
  switch (stype) {
   case sig_ramp : {
    float cut_point_exact = period_samples * duty_cycle;
    for (uint32_t i=0;i<wave_length_exact_int;i++) {
     float cyc_fraction = fmod(i,period_samples) + phase;
     float sv = (cyc_fraction < cut_point_exact) ?
	     cyc_fraction / cut_point_exact :
	     (period_samples - cyc_fraction) / (period_samples - cut_point_exact);
     // f2(sv,cyc_fraction);
     int32_t si = sv * amplitude + offset;
     si = CLAMP16I(si);
     stored_waveform[i] = si;
    }
    break;
   }
   case sig_pulse : {
    float turn_off_exact = duty_cycle * period_samples;
    // f1(turn_off_exact);
    for (uint32_t i=0;i<wave_length_exact_int;i++) {
     float fraction = fmod(i,period_samples) + phase;
     int sv = (fraction < turn_off_exact);
     int32_t si = sv * amplitude + offset;
     si = CLAMP16I(si);
     // d2(i,si);
     stored_waveform[i] = si;
    }
    break;
   }
   case sig_sine : {
    for (uint32_t i=0;i<wave_length_exact_int;i++) {
     float angle = 2 * PI * (float)i / period_samples + phase;
     while (angle > (2.0*PI))  { angle -= 2.0 * PI; };
     float   sv = sin(angle);
     int32_t si = sv * amplitude + offset;
     si = CLAMP16I(si);
     stored_waveform[i] = si;
    }
    // for (uint32_t i=0;i<wave_length_exact_int;i++) {
    //  d2c(i,stored_waveform[i]);
    // }         
    break;
   }
   default : {
   }
  }
 }
 is_setup = true;
}


void
makesig_c::set_ramp(float f, int16_t a, int16_t o, float w) {
 frequency = f;
 amplitude = a;
 offset = o;
 stype = sig_ramp;
 if (w < 0)      { w = 0; }
 else if (w > 1) { w = 1;};
 duty_cycle = w;
 is_setup = false;
};


void
makesig_c::set_pulse(float f, int16_t a, int16_t o, float w) {
 frequency = f;
 amplitude = a;
 offset = o;
 stype = sig_pulse;
 if (w < 0)      { w = 0; }
 else if (w > 1) { w = 1;};
 duty_cycle = w;
 is_setup = false;
};

void 
makesig_c::set_square(float f, int16_t a, int16_t o) {
 frequency = f;
 amplitude = a;
 offset = o;
 stype = sig_pulse;
 duty_cycle = 0.5;
 is_setup = false;
};
  
void
makesig_c::set_sine(float f, int16_t a, int16_t o, float ph) {
 frequency = f;
 amplitude = a;
 offset = o;
 phase = ph;
 stype = sig_sine;
 is_setup = false;
};


void makesig_c::work() {
 if (!is_setup) {
  setup();
 }

 if (!wave_on_fly) {
  uint32_t p_samps = stored_waveform.size();
  for (uint32_t i=0;i<l;i++) {
   uint32_t idx = cpos % p_samps;
   int32_t si = stored_waveform[idx];
   (*out)[i] = si;
   // d3c(cpos,si,idx);
   cpos++;
  }
 } else {
  std::cout << "wave on fly\n";
  uint32_t period_samples_int = floor(period_samples+0.5);
  float cut_exact = period_samples * duty_cycle;
  for (uint32_t i=0;i<l;i++) {
   float cyc_fraction = fmod(cpos,period_samples);
   float sv;
   switch (stype) {
    case sig_ramp: {
      sv = (cyc_fraction < cut_exact) ? 
       cyc_fraction / cut_exact :
       (period_samples-cyc_fraction) / (period_samples - cut_exact);
      break;
    }
    case sig_pulse : {
      sv = (cyc_fraction < cut_exact) ? 1 : 0;
      break;
    }
    case sig_sine : {
     float angle = (2 * PI * (float)(cpos % period_samples_int) / (float)period_samples_int) + phase;
     sv = sin(angle);
     break;
    }
    default : {
     sv = 0;
    }
   }
   int32_t si = amplitude * sv + offset;
   si = CLAMP16I(si);
   (*out)[i] = si;
   cpos++;
  }
 };
};


} // namespace

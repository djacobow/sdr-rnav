// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz

#include "ditdah.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

// There are two classes in this file. One is for "thresholding" with 
// hysteresis. It just converts a floating point time series into a 
// binary 1/0 time series. (Note that we should probably do this with
// fixed point but I don't have the fixed point version of the envelope
// follower working yet.

namespace djdsp {

thresh_c::thresh_c() {
 lthresh = DEFAULT_LO_THRESHOLD;
 hthresh = DEFAULT_HI_THRESHOLD;
 auto_thresh = false;
 last_val = 0;
 c_max = 0;
 c_min = 0x7fff;
 l_c_max = 0;
 l_c_min = 0x7fff;
};

void 
thresh_c::set_thresholds(int16_t l, int16_t h) {
 lthresh = l;
 hthresh = h;
 auto_thresh = false;
};

void
thresh_c::set_autothresh(bool a) {
 auto_thresh = a;
};

void 
thresh_c::work() {
 uint32_t len  = l;

 for (uint32_t i=0;i<len;i++) {
  int16_t bval = (*in)[i];

  // Here's a bit of highly questionable code. Tries to guess what the
  // "high" and "low" thresholds for overcoming hysteresis should be
  // from the sample flow. Here we base it on the all-time high and 
  // low values in the input stream -- probably not a good idea in the 
  // presence of noise. Better plan is to track max and min over some 
  // number of samples
  if (auto_thresh) {
   if (bval > c_max) { c_max = bval; };
   if (bval < c_min) { c_min = bval; };
   int32_t max = (c_max + l_c_max) >> 1;
   int32_t min = (c_min + l_c_min) >> 1;
   int16_t range = max - min;
   if (range<=0) { 
    range = 0;
    lthresh = c_min + 50;
    hthresh = c_max - 50;
   } else {   
    lthresh = min + 0.2 * range; // was 0.4 7/14/2013
    hthresh = max - 0.6 * range; // was 0.4 7/14/2013
   };
  }
  bool nv = (bval > hthresh) ? 0x1 : (bval < lthresh) ? 0x0 : last_val;

  (*out)[i] = nv;
  last_val = nv;
 };
 l_c_max = c_max;
 l_c_min = c_min;
};


ditdah_c::ditdah_c() {
 sample_rate  = DEFAULT_SAMPLE_RATE;
 min_dit_tm   = MIN_DIT_TIME;
 min_dah_tm   = MIN_DAH_TIME;
 min_sep_tm   = MIN_SEP_TIME;
 min_space_tm = MIN_SPACE_TIME;
 max_space_tm = MAX_SPACE_TIME;
 setup();
}

void ditdah_c::set_sample_rate(uint32_t sr) { sample_rate = sr; setup();}
void ditdah_c::set_min_dit(float md)   { min_dit_tm = md; setup(); }
void ditdah_c::set_min_dah(float md)   { min_dah_tm = md; setup(); }
void ditdah_c::set_min_space(float sp) { min_space_tm = sp; setup(); }
void ditdah_c::set_max_space(float sp) { max_space_tm = sp; setup(); }
void ditdah_c::set_min_sep(float sp)   { min_sep_tm = sp; setup(); }
uint32_t ditdah_c::lastElems() {
 return last_elems;
}



void
ditdah_c::setup() {
 min_dit_sp   = uint32_t(min_dit_tm   * (float)sample_rate);
 min_dah_sp   = uint32_t(min_dah_tm   * (float)sample_rate);
 min_space_sp = uint32_t(min_space_tm * (float)sample_rate);
 min_sep_sp   = uint32_t(min_sep_tm   * (float)sample_rate);
 max_space_sp = uint32_t(max_space_tm * (float)sample_rate);

 d3(min_dit_sp,min_dah_sp,min_space_sp);
 d2(min_sep_sp,max_space_sp);

 current_position = 0;
 new_position = 0;
 push_up_ct = 0;
 push_down_ct = 0;
 flip_up_thresh = (uint16_t)(DEFAULT_FLIP_UP_THRESH_TIME * (float)sample_rate);
 flip_down_thresh = (uint16_t)(DEFAULT_FLIP_DOWN_THRESH_TIME * (float)sample_rate);
 last_elems = 0;
};

void
ditdah_c::work() {
  uint32_t len = l;

  uint8_t symcount = 0;
  for (uint32_t i=0;i<len;i++) {
   bool nv = (*in)[i]; 

   // std::cout << "xxx: " << (current_position ? '1' : '0') 
   //  << " position_time: " << position_time
   // << std::endl;

   if (!current_position) {
    if (nv) {
      push_up_ct++;
    } else {
      if (push_up_ct) push_up_ct--;
    }
    if (push_up_ct >= flip_up_thresh) {
      new_position = 1;
      push_up_ct = 0;
      push_down_ct = 0;
    }
   } else {
    if (nv) {
     if (push_down_ct) push_down_ct--;
    } else {
     push_down_ct++;
    }
    if (push_down_ct >= flip_down_thresh) {
     new_position = 0;
     push_down_ct = 0;
     push_up_ct = 0;
    }
   };

   if (new_position != current_position) { 
    if (current_position) {
     if (position_time > min_dah_sp) {
      (*out)[symcount++] = '-';
     } else if (position_time > min_dit_sp) {
      (*out)[symcount++] = '.';
     }
    } else {
     if (position_time >= min_space_sp) {
      (*out)[symcount++] = 'S';
     } else if (position_time >= min_sep_sp) {
      (*out)[symcount++] = ' ';
     }
    }
    position_time = 0;
   } else {
    if (position_time > max_space_sp) {
     (*out)[symcount++] = 'S';
     position_time = 0;
    }
    position_time++;
   }
   current_position = new_position;
  }
  (*out)[symcount] = 0;
  last_elems = symcount;
};

} // namespace

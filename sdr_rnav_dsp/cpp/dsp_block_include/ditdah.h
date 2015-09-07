

// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef DITDAH_H
#define DITDAH_H

#ifndef DEFAULT_LO_THRESHOLD
#define DEFAULT_LO_THRESHOLD (225)
#endif

#ifndef DEFAULT_HI_THRESHOLD 
#define DEFAULT_HI_THRESHOLD (375)
#endif

#ifndef MIN_DIT_TIME
#define MIN_DIT_TIME (0.05)
#endif

#ifndef MIN_DAH_TIME
// was 0.24
#define MIN_DAH_TIME (0.20)
#endif

#ifndef MIN_SEP_TIME
#define MIN_SEP_TIME (0.24)
#endif

#ifndef MIN_SPACE_TIME
#define MIN_SPACE_TIME (3)
#endif

#ifndef MAX_SPACE_TIME
#define MAX_SPACE_TIME (4)
#endif

#ifndef DEFAULT_SAMPLE_RATE
#define DEFAULT_SAMPLE_RATE (500)
#endif

#ifndef DEFAULT_FLIP_UP_THRESH_TIME
#define DEFAULT_FLIP_UP_THRESH_TIME (0.04)
#endif

#ifndef DEFAULT_FLIP_DOWN_THRESH_TIME
#define DEFAULT_FLIP_DOWN_THRESH_TIME (0.04)
#endif


#include "block_base.h"

namespace djdsp {


class thresh_c : public block_base_c {
 public:
  void work();
  thresh_c();
  void set_thresholds(int16_t l, int16_t h);
  void set_autothresh(bool a);
 private:
  int16_t lthresh;
  int16_t hthresh;
  int16_t last_val;
  int16_t c_max;
  int16_t c_min;
  int16_t l_c_max;
  int16_t l_c_min;
  bool auto_thresh;
};


class ditdah_c : public block_base_c {
 public:
  void work();
  // void show_syms(ditdah_symbols_t *buf, uint8_t ct);
  ditdah_c();
  uint32_t lastElems();
  void set_sample_rate(uint32_t sr);
  void set_min_dit(float md);
  void set_min_dah(float md);
  void set_min_space(float sp);
  void set_max_space(float sp);
  void set_min_sep(float sp);
  
 private:
  void setup();
  uint16_t min_dit_sp;
  uint16_t min_dah_sp;
  uint16_t min_space_sp;
  uint16_t max_space_sp;
  uint16_t min_sep_sp;
  float min_dit_tm;
  float min_dah_tm;
  float min_space_tm;
  float max_space_tm;
  float min_sep_tm;
  uint16_t sample_rate;
  uint32_t last_elems;
  
  // for work
  bool current_position;
  bool new_position;
  uint16_t push_up_ct;
  uint16_t push_down_ct;
  uint16_t flip_up_thresh;
  uint16_t flip_down_thresh;
  uint16_t position_time;

};

} // namespace

#endif



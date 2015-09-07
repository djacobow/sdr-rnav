
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_DITDAH_H
#define DP_DITDAH_H

#ifndef MIN_DIT_TIME
#define MIN_DIT_TIME (0.05)
#endif

#ifndef MIN_DAH_TIME
#define MIN_DAH_TIME (0.20)
#endif

#ifndef MIN_SEP_TIME
#define MIN_SEP_TIME (0.24)
#endif

#ifndef MIN_SPACE_TIME
#define MIN_SPACE_TIME (2)
#endif

#ifndef MAX_SPACE_TIME
#define MAX_SPACE_TIME (3)
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

#include "dp_base_internals.h"

DP_SUB_CREATOR_DECL(ditdah);
DP_FN_DECL(ditdah,init);
DP_FN_DECL(ditdah,prerun);
DP_FN_DECL(ditdah,work);
DP_SUB_GENERIC_GETTER_DECL(ditdah,last_elems,uint32_t);
DP_SUB_GENERIC_SETTER_DECL(ditdah,sample_rate, uint32_t);
DP_SUB_GENERIC_SETTER_DECL(ditdah,min_dit,float);
DP_SUB_GENERIC_SETTER_DECL(ditdah,min_dah,float);
DP_SUB_GENERIC_SETTER_DECL(ditdah,min_space,float);
DP_SUB_GENERIC_SETTER_DECL(ditdah,max_space,float);
DP_SUB_GENERIC_SETTER_DECL(ditdah,min_sep,float);
DP_SUB_GENERIC_SETTER_DECL(ditdah,autolenout,uint32_t *);

typedef struct dp_ditdah_sub_t {
  uint16_t min_dit_sp;
  uint16_t min_dah_sp;
  uint16_t min_space_sp;
  uint16_t max_space_sp;
  uint16_t min_sep_sp;
  float min_dit;
  float min_dah;
  float min_space;
  float max_space;
  float min_sep;
  uint16_t sample_rate;
  uint32_t last_elems;
  dp_bool_t current_position;
  dp_bool_t new_position;
  uint16_t push_up_ct;
  uint16_t push_down_ct;
  uint16_t flip_up_thresh;
  uint16_t flip_down_thresh;
  uint16_t position_time;
  uint32_t *autolenout;
} dp_ditdah_sub_t;

#endif



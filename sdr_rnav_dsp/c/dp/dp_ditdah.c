/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz
*/
#include "dp_ditdah.h"
#include <stdlib.h>

DP_SUB_CREATOR_IMPL(ditdah)

DP_SUB_GENERIC_SETTER_IMPL(ditdah,sample_rate,uint32_t)
DP_SUB_GENERIC_SETTER_IMPL(ditdah,min_dit,float)
DP_SUB_GENERIC_SETTER_IMPL(ditdah,min_dah,float)
DP_SUB_GENERIC_SETTER_IMPL(ditdah,min_space,float)
DP_SUB_GENERIC_SETTER_IMPL(ditdah,max_space,float)
DP_SUB_GENERIC_SETTER_IMPL(ditdah,min_sep,float)
DP_SUB_GENERIC_GETTER_IMPL(ditdah,last_elems, uint32_t)
DP_SUB_GENERIC_SETTER_IMPL(ditdah,autolenout, uint32_t *)

DP_FN_PREAMBLE(ditdah,init) {
 s->sample_rate  = DEFAULT_SAMPLE_RATE;
 s->min_dit   = MIN_DIT_TIME;
 s->min_dah   = MIN_DAH_TIME;
 s->min_sep   = MIN_SEP_TIME;
 s->min_space = MIN_SPACE_TIME;
 s->max_space = MAX_SPACE_TIME;
 s->autolenout = 0;
 b->sub_work = &dp_ditdah_work;
 b->sub_prerun = &dp_ditdah_prerun;
 dp_ditdah_prerun(b);
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(ditdah,prerun) {
 s->min_dit_sp   = (uint32_t)(s->min_dit   * (float)s->sample_rate);
 s->min_dah_sp   = (uint32_t)(s->min_dah   * (float)s->sample_rate);
 s->min_space_sp = (uint32_t)(s->min_space * (float)s->sample_rate);
 s->min_sep_sp   = (uint32_t)(s->min_sep   * (float)s->sample_rate);
 s->max_space_sp = (uint32_t)(s->max_space * (float)s->sample_rate);

 s->current_position = 0;
 s->new_position = 0;
 s->push_up_ct = 0;
 s->push_down_ct = 0;
 s->flip_up_thresh = (uint16_t)(DEFAULT_FLIP_UP_THRESH_TIME * 
		     (float)s->sample_rate);
 s->flip_down_thresh = (uint16_t)(DEFAULT_FLIP_DOWN_THRESH_TIME * 
		        (float)s->sample_rate);
 s->last_elems = 0;
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(ditdah,work) {
  uint32_t len = b->runlength;
  uint8_t symcount = 0;
  uint32_t i;
  for (i=0;i<len;i++) {
   dp_bool_t nv = b->in_v->v[i]; 

   /* std::cout << "xxx: " << (current_position ? '1' : '0') 
   //  << " position_time: " << position_time
   // << std::endl;
   */
   if (!s->current_position) {
    if (nv) {
      s->push_up_ct++;
    } else {
      if (s->push_up_ct) s->push_up_ct--;
    }
    if (s->push_up_ct >= s->flip_up_thresh) {
      s->new_position = 1;
      s->push_up_ct = 0;
      s->push_down_ct = 0;
    }
   } else {
    if (nv) {
     if (s->push_down_ct) s->push_down_ct--;
    } else {
     s->push_down_ct++;
    }
    if (s->push_down_ct >= s->flip_down_thresh) {
     s->new_position = 0;
     s->push_down_ct = 0;
     s->push_up_ct = 0;
    }
   };

   if (s->new_position != s->current_position) { 
    if (s->current_position) {
     if (s->position_time > s->min_dah_sp) {
      b->out_v->v[symcount++] = '-';
     } else if (s->position_time > s->min_dit_sp) {
      b->out_v->v[symcount++] = '.';
     }
    } else {
     if (s->position_time >= s->min_space_sp) {
      b->out_v->v[symcount++] = 'S';
     } else if (s->position_time >= s->min_sep_sp) {
      b->out_v->v[symcount++] = ' ';
     }
    }
    s->position_time = 0;
   } else {
    if (s->position_time > s->max_space_sp) {
     b->out_v->v[symcount++] = 'S';
     s->position_time = 0;
    }
    s->position_time++;
   }
   s->current_position = s->new_position;
  }
  b->out_v->v[symcount] = 0;
  s->last_elems = symcount;
  if (s->autolenout) {
   *(s->autolenout) = symcount;
  }
}
DP_FN_POSTAMBLE


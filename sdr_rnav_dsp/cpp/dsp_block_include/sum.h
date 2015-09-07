
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz

#ifndef SUM_H 
#define SUM_H 

#include "block_base.h"

namespace djdsp {

typedef struct sum_stream_t {
 dvec_t  *vector;
 float   scale_f;
 int16_t scale_i;
} sum_stream_t;

typedef std::vector<sum_stream_t> sum_dvecs_t;

class sum_c : public block_base_c {
 public:
  sum_c();
  void work();
  void add_addend(dvec_t *i, float scale);
  void drop_addends();
 private:
  sum_dvecs_t dvecs;
};

} // namespace

#endif



// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz

#ifndef MULTIPLY_H 
#define MULTIPLY_H 

#include "block_base.h"

namespace djdsp {

typedef std::vector<dvec_t *> mult_dvecs_t;

class multiply_c : public block_base_c {
 public:
  multiply_c();
  void work();
  void add_factor(dvec_t *i);
  void drop_factors();
 private:
  mult_dvecs_t dvecs;
};

} // namespace

#endif



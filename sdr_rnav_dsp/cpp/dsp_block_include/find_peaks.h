
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef FINDPEAKS_H 
#define FINDPEAKS_H

#include "block_base.h"

namespace djdsp {

typedef struct peak_pt_t {
 int32_t bin;
 float   db;
} peak_pt_t;

typedef struct peak_pts_t {
 std::vector<peak_pt_t> points;
 uint32_t length;
 int32_t average;
 uint32_t iteration;
} peak_pts_t;

class findpeaks_c : public block_base_c {

 public:
  findpeaks_c();
  ~findpeaks_c();
  void work();
  void pre_run();
  void set_out(peak_pts_t *ipts);
 private:
  float ln10;
  peak_pts_t *pts;
  uint32_t iter;
};


} // namespace

#endif


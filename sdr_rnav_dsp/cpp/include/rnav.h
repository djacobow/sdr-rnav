
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef RNAV_H
#define RNAV_H

// This contains datastructures and routines for the "rnav" portion
// of the program, which I will define here as 
//  1) keeping track of what VORs have been received recently
//  2) using some or all of them to compute a position


#include "vordb.h"
#include <ctime>
#include <string>
#include <vector>

#define MAX_AGE  (30)
#define MAX_VORS (5)
#define MIN_STRENGTH (15.0)

typedef std::vector<uint32_t> flist_t;

typedef struct vor_received_t {
 bool   valid;
 vor_data_t *info;
 uint32_t freq_khz;
 float  signal_strength;
 float  radial_deg;
 time_t last_received; 
} vor_received_t;

typedef vor_received_t vors_received_t[MAX_VORS];


class vors_stat_c {
 public:
  vors_stat_c();
  ~vors_stat_c();
  void update_vor(const char *, const uint32_t freq, const float radial, const float strength);
  int purge_old(int32_t age);
  flist_t getFreqs();
  std::string show();
 private:
  uint8_t max_vors;
  float min_strength;
  int32_t max_age;

  vors_received_t vorlist;
  void init(vor_received_t &v);
};



#endif


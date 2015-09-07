
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#include "rnav.h"
#include <iostream>
#include <sstream>
#include <ctime>

vors_stat_c::vors_stat_c() {
 for (uint32_t i=0;i<MAX_VORS;i++) {
  init(vorlist[i]);
 }
 min_strength = MIN_STRENGTH;
 max_vors = MAX_VORS;
 max_age = MAX_AGE;
};

vors_stat_c::~vors_stat_c() {
 // should not be much to do here
}

void
vors_stat_c::init(vor_received_t &v) {
 v.valid = false;
 v.info  = 0;
 v.signal_strength = 0;
 v.radial_deg = 0;
 v.last_received = 0;
};

flist_t 
vors_stat_c::getFreqs() {
 flist_t freqs;
 for (uint8_t i=0;i<max_vors;i++) {
  if (vorlist[i].valid) {
   uint32_t freq = vorlist[i].freq_khz;
   freqs.push_back(freq);
  }
 }
 return freqs;
};


void
vors_stat_c::update_vor(const char *iid, const uint32_t ifreq,
		        const float iradial, const float istrength) {

 uint32_t freq_khz = ifreq / 1000; 
 vor_data_t *info = find_for_matching_id_freq(iid,freq_khz);

 if (1) {
  int8_t repl_idx = -1;
  time_t now    = time(0);
  time_t earliest_time = now;
  int8_t earliest_idx  = -1;
  int8_t last_invalid = -1;
  if (istrength > min_strength) {
   for (uint8_t i=0;i<max_vors;i++) {
    if (vorlist[i].last_received < earliest_time) {
     earliest_time = vorlist[i].last_received;
     earliest_idx = i;
    } 
    if (!vorlist[i].valid) { last_invalid = i; }
    //
    // first, if there is already an entry for this VOR, use it
    if (freq_khz == vorlist[i].freq_khz) { repl_idx = i; }
   }
   // if there is no entry for this VOR, look for an entry marked
   // invalid
   if (repl_idx < 0) { repl_idx = last_invalid; }
   // finally, if there is no invalid entry, replace the oldest
   if (repl_idx < 0) { repl_idx = earliest_idx; }

   vorlist[repl_idx].info            = info;
   vorlist[repl_idx].signal_strength = istrength;
   vorlist[repl_idx].radial_deg      = iradial;
   vorlist[repl_idx].last_received   = now;
   vorlist[repl_idx].valid           = true;
  }
 };
 purge_old(max_age);
}

int
vors_stat_c::purge_old(int32_t age) {
 int pcount = 0;
 for (uint8_t i=0;i<max_vors;i++) {
  time_t now = time(0);
  if (now - vorlist[i].last_received > age) {
   vorlist[i].valid = 0;
   pcount++;
  }
 }
 return pcount;
};

std::string
vors_stat_c::show() {
 std::stringstream ss;
 time_t now = time(0);
 for (int i=0;i<max_vors;i++) {
  vor_received_t vor = vorlist[i];
  ss << (vor.valid ? "valid" : "n/a  ")
     << " r"
     << vor.radial_deg
     << ' ' 
     << vor.signal_strength
     << " age:"
     << (now - vor.last_received);

  if (vor.info) {
   ss << " lat:"
      << vor.info->latitude
      << " lon:"
      << vor.info->longitude
      << " freq:"
      << vor.info->frequency
      << " ss:"
      << vor.info->id;
  }

  ss  << std::endl;
 };
 return ss.str();
};


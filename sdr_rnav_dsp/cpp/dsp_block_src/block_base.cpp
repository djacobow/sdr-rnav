
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#include "block_base.h"
#include <iostream>

// Implementation of base "block" class for all DSP blocks. Contains
// setters and getters for input output buffers and a way to name
// instances, as well as default functions that may or may not be 
// overridden in a given subclass.
namespace djdsp {

block_base_c::block_base_c() { 
 in         = 0;
 out        = 0;
 name.clear();
 group_mask = 0;
 l_set      = false;
 l          = 0;
 is_complex = false;
 complex_ok = false;
};

block_base_c::~block_base_c() { };

void block_base_c::set_in(const dvec_t *i) {
 in = i;
 if (in && !l_set) {
  l = in->size();
  l_set = true;
 }
}

void block_base_c::set_in(const dvec_t *i, uint32_t il) {
 in = i;
 l  = il;
 l_set = true;
};

void block_base_c::set_in(const dvec_t *i, uint32_t il, sig_cpx_t s) {
 in = i;
 l = il;
 l_set = true;
 is_complex = s == s_complex;
};


void block_base_c::set_complex(sig_cpx_t s) {
 is_complex = s == s_complex;
 if (is_complex && !complex_ok) {
  std::cout << "-warn- (" << name 
	    << ") module does not support complex data" 
	    << std::endl;
  is_complex = false;
 }
};

void block_base_c::set_out(dvec_t *o) {
 out = o;
}

void block_base_c::pre_run() { };
void block_base_c::post_run() { };

void block_base_c::set_name(std::string n) {
 name = n;
}
std::string block_base_c::get_name() { return name; }

void block_base_c::set_runlen(uint32_t rl) {
 l = rl;

 if (in) {
  uint32_t vl = in->size();
  if (rl > vl) {
   std::cout << "-error- run length (" << rl 
             << ") given exceeds length of input vector (" << vl
             << ")" << std::endl;
   exit(-1);
  }
 } else if (out) {
  uint32_t vl = out->size();
  if (rl > vl) {
   std::cout << "-error- run length (" << rl 
             << ") given exceeds length of output vector (" << vl
             << ")" << std::endl;
  }
 }
 l_set = true;
};

void block_base_c::run(uint32_t rmask) {
 if (l_set && (rmask & group_mask)) { 
  work();
 } else {
 }
};

void block_base_c::work() { };

} // namespace

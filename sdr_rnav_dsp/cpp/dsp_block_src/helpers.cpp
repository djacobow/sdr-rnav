

// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#include "helpers.h"
#include <iostream>

// These routines are useful for debug. The print out all the values in
// a vector.

namespace djdsp {

void show_dv (dvec_t d) {
 uint32_t l = d.size();
 std::cout << "==== buffer len " << l << std::endl;
 for (uint32_t i=0; i<l; i++) {
  std::cout << d[i] << " ";
 }
 std::cout << std::endl;
};

void show_dv (dvec_t d, uint32_t l) {
 uint32_t bl = d.size();
 std::cout << "==== buffer len " << bl << " showing " << l << std::endl;
 for (uint32_t i=0; i<l; i++) {
  std::cout << d[i] << " ";
 }
 std::cout << std::endl;
};

} // namespace

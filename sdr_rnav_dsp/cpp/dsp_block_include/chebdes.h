
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef CHEBDES_H
#define CHEBDES_H

#include <inttypes.h>

namespace djdsp {

bool chebdes_i(double fc, bool  lh, double pr, uint16_t np, 
	       int16_t *a, int16_t *b);
bool chebdes_d(double fc, bool  lh, double pr, uint16_t np, double *a, double *b);
void cd_helper(double fc, bool lh, double pr, uint16_t np, double p,
	       double *a0, double *a1, double *a2, double *b1, double *b2);

} // namespace

#endif




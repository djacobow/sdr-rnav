// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz

// coefficients for some FIR filters frequently used
// 
// These filters were calculated using an online tool and then
// placed here. However, this file has since been obviated in most
// cases by the use of the local automatic generation of filter
// coefficients. 

#ifndef _FILTER_KERNELS_H
#define _FILTER_KERNELS_H

#include <inttypes.h>


#define BP1020_SR8k_LEN  (143)

extern const uint16_t len_BP1020_SR8k;
extern const int16_t  coeffs_BP_1020_SR8k[ BP1020_SR8k_LEN ];

#define LP25k_SR256k_LEN (67)
extern const uint16_t len_LP25k_SR256k;
extern const int16_t  coeffs_LP_25k_SR256k [ LP25k_SR256k_LEN ];

#define LP25k_SR250k_LEN (65)
extern const uint16_t len_LP25k_SR250k;
extern const int16_t  coeffs_LP_25k_SR250k [ LP25k_SR250k_LEN ];

extern const uint16_t len_dummy;
extern const int16_t coeffs_dummy[2];

#define LP5k_SR31250_LEN (79)

extern const uint16_t len_LP5k_SR31250;
extern const int16_t coeffs_LP_5k_SR31250[ LP5k_SR31250_LEN ];

#define BP1020_SR6250_LEN (65)

extern const uint16_t len_BP1020_SR6250;
extern const int16_t coeffs_BP_1020_SR6250[ BP1020_SR6250_LEN ];


#define LP30_SR6250_LEN (363) 

extern const uint16_t len_LP30_SR6250;
extern const int16_t coeffs_LP_30_SR6250[ LP30_SR6250_LEN ];

#define LP30_SR31250_LEN (659) 

extern const uint16_t len_LP30_SR31250;
extern const int16_t coeffs_LP_30_SR31250[ LP30_SR31250_LEN ];

#define LP400_SR31250_LEN (315)

extern const uint16_t len_LP400_SR31250;
extern const int16_t coeffs_LP_400_SR31250[ LP400_SR31250_LEN ];


#define BP30_SR1250_LEN (383)

extern const uint16_t len_BP30_SR1250;
extern const int16_t coeffs_BP_30_SR1250[ BP30_SR1250_LEN ];

#define BP9960_SR31250_LEN (145)

extern const uint16_t len_BP9960_SR31250;
extern const int16_t coeffs_BP_9960_SR31250[ BP9960_SR31250_LEN ];

#define BP100_SR1250_LEN (161)
extern const uint16_t len_BP100_SR1250;
extern const int16_t coeffs_BP_100_SR1250 [ BP100_SR1250_LEN ];

#endif

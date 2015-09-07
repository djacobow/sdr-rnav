
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef DP_MORSEDEC_H
#define DP_MORSEDEC_H

#include "dp_base_internals.h"

#define DP_MORSE_MAX_STR (20)

DP_SUB_CREATOR_DECL(morse);
DP_FN_DECL(morse,init);
DP_FN_DECL(morse,deinit);
DP_FN_DECL(morse,work);
void dp_morse_get_decoded(dp_base_t *b, char *);
void dp_morse_set_autolen(dp_base_t *b, uint32_t *al);

typedef struct dp_morse_sub_t {
  int8_t tpos;
  char *decoded_str;
  uint32_t last_elems;
  uint32_t strpos;
  uint32_t *autolen;
} dp_morse_sub_t;

#endif


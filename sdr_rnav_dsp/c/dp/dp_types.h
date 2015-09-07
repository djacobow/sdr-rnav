
#ifndef __DP_TYPES_H
#define __DP_TYPES_H

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

typedef int16_t dp_int_t;
typedef enum { dp_false = 0, dp_true } dp_bool_t;
typedef float   dp_float_t;

#ifndef true
#define true dp_true
#endif

#ifndef false
#define false dp_false
#endif

#endif


#ifndef __VORDB_H
#define __VORDB_H

#include <inttypes.h>

typedef struct vor_data_t {
   uint32_t	frequency;
  float	magnetic_variation;
  float	longitude;
  float	elevation;
  float	latitude;
  char	id[4];

} vor_data_t;

extern vor_data_t vor_data[];
extern uint32_t vor_count;

vor_data_t *find_vor_matching_id_freq(const char *, uint32_t);

#endif



/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef __CTRL_THREAD_H
#define __CTRL_THREAD_H

#include "receiver_stat.h"

#define SHORT_STRING_LENGTH (128)
#define LONG_STRING_LENGTH  (2048)

/* minimum number of buffers to hang on one frequency
// (to let receiver settle) to determine if station is 
// there
*/
#define MIN_DWELL_BLOCKS (4) 
/* minimum time to sit on a frequency to be sure to pick
// up a complete ID
*/
#define ID_DWELL_TIME (35)

void *ctrl_thread_fn(void *f);
void stats_to_json(char *dst, receiver_stat_t *prs, char *idstr);

/* borrowed from openBSD */
size_t strlcat(char *dst, const char *src, size_t siz);


#endif


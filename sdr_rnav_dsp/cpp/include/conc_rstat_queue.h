
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef __CONC_RSTAT_QUEUE_H
#define __CONC_RSTAT_QUEUE_H

#include "conc_queue.h"
#include "receiver_stat.h"

typedef concurrent_queue_c<receiver_stat_t> concurrent_rstat_queue_c;

#endif


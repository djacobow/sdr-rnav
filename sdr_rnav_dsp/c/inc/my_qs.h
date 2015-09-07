
#ifndef MY_QS_H
#define MY_QS_H

#include <pthread.h>
#include "dp_q_internals.h"
#include "dp_conc_q_internals.h"
#include "dp_findpeaks.h"
#include "receiver_stat.h"

typedef char dp_str80_t[80];

DP_QUEUE_INSTANTIATE_HEADER(peaks,peak_pts_t);
DP_CONC_QUEUE_INSTANTIATE_HEADER(peaks,peak_pts_t);

DP_QUEUE_INSTANTIATE_HEADER(char,char);
DP_CONC_QUEUE_INSTANTIATE_HEADER(char,char);

DP_QUEUE_INSTANTIATE_HEADER(str80,dp_str80_t);
DP_CONC_QUEUE_INSTANTIATE_HEADER(str80,dp_str80_t);

DP_QUEUE_INSTANTIATE_HEADER(rstat,receiver_stat_t);
DP_CONC_QUEUE_INSTANTIATE_HEADER(rstat,receiver_stat_t);

#endif



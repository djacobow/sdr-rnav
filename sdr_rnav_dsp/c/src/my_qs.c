
#include <stdio.h>
#include "my_qs.h"

DP_QUEUE_INSTANTIATE_IMPL(str80,dp_str80_t)
DP_CONC_QUEUE_INSTANTIATE_IMPL(str80,dp_str80_t)

DP_QUEUE_INSTANTIATE_IMPL(rstat,receiver_stat_t)
DP_CONC_QUEUE_INSTANTIATE_IMPL(rstat,receiver_stat_t)

DP_QUEUE_INSTANTIATE_IMPL(peaks,peak_pts_t)
DP_CONC_QUEUE_INSTANTIATE_IMPL(peaks,peak_pts_t)

DP_QUEUE_INSTANTIATE_IMPL(char,char)
DP_CONC_QUEUE_INSTANTIATE_IMPL(char,char)


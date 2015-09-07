
/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2014
//
// Copyright 2014, David Jacobowitz
*/

#ifndef _DP_RADIO_fcdpp_H
#define _DP_RADIO_fcdpp_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "dp_conc_ringbuffer.h"
#include "dp_base_internals.h"
#include "dp_fcdpp_wrap.h"

/*
 This is an interface class for controlling and pulling data from the
 Funcube Dongle Pro+.
 */

typedef struct dp_radiofcdpp_sub_t {
  dp_conc_ringbuffer_t rb;
  fcdpp_wrapper_t *dev;
  dp_bool_t dev_is_open;
  dp_bool_t running_async;     /* true of async thread still running */
  dp_bool_t report_overflow;   /* true if we want to hear about write 
				  overflows */
  uint32_t ec;                 /* accumulated error codes, if any */
  uint32_t curr_freq;
  char *devname;
} dp_radiofcdpp_sub_t;


DP_SUB_CREATOR_DECL(radiofcdpp);
DP_FN_DECL(radiofcdpp,init);
DP_FN_DECL(radiofcdpp,deinit);
DP_FN_DECL(radiofcdpp,prerun);
DP_FN_DECL(radiofcdpp,work);
DP_FN_DECL(radiofcdpp,postrun);
pthread_t *dp_radiofcdpp_start_thread(dp_base_t *);

int dp_radiofcdpp_get_tuner_gains(dp_base_t *b);

DP_SUB_GENERIC_SETTER_DECL(radiofcdpp,dev,fcdpp_wrapper_t *);
int dp_radiofcdpp_open_device(dp_base_t *, char *n);
int dp_radiofcdpp_close_device(dp_base_t *);
/* int dp_radiofcdpp_list_devices(dp_base_t *); */
void dp_radiofcdpp_set_buffers(dp_base_t *b, uint32_t blen, uint32_t bcount);
DP_FN_DECL(radiofcdpp,stop_async);
dp_bool_t *dp_radiofcdpp_get_running_async(dp_base_t *b);
fcdpp_wrapper_t *dp_radiofcdpp_get_dev(dp_base_t *b);
DP_SUB_GENERIC_GETTER_DECL(radiofcdpp,curr_freq,uint32_t);
DP_SUB_GENERIC_GETTER_DECL(radiofcdpp,report_overflow,dp_bool_t);
dp_conc_ringbuffer_t *dp_radiofcdpp_get_rb(dp_base_t *b);

int dp_radiofcdpp_set_frequency(dp_base_t *b, uint32_t a);

/* function to call to start the second thread */
void *dp_fcdpp_wrapper_thread_fn(void *f);

#endif


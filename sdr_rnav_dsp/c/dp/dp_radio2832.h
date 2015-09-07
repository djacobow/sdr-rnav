
/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef _DP_RADIO_2832_H
#define _DP_RADIO_2832_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "dp_conc_ringbuffer.h"
#include "dp_base_internals.h"

#include <rtl-sdr.h>
/*

 This is an interface class for controlling and pulling data from the
 RTL dongle.

 This class is like ther other dsp block_base_c classes, but it's
 different.  For one, it has some ugly gunk because it's c++, but must
 interface with a callback function in pure C. The result is that it
 has some getters that hand over important internal guts.

 More importantly, rather than using the set_output() function ones to
 store and capture a pointer to a buffer that will receive the output of
 the block when work() is called this class instead uses a set_consumer()
 function which takes the address of the next object to use the data. Then
 in work() after each buffer of data is ready, it calls the consumer
 object's set_in().

 This avoids a memcpy() or similar.

 Another complicating factor of this block is that it "straddles"
 two threads, really three implicitly. The object setup stuff and
 functions like work() will be in one thread. The callbacks from the
 RTL-SDR library will run in a separate thread. The two are synchronized
 through mutexes. In particular, work() will wait on input buffers
 being ready, but the callback will /not/ wait on the output buffer,
 it'll just clobber it. This is probably what we want; we don't want
 the radio to fall behind writing, we'll just keep up and let bygones
 be bygones. The third thread is only implied, but clearly calling the
 RTL-SDR library calls for setting up the asynchronous callbacks also
 starts a new thread in that library.

 */


/* Various command types to be sent to the RTL device */
typedef enum rtl2832_cmd_t {
 ST_FREQ = 1,
 GT_FREQ,
 ST_SR,
 GT_SR,
 ST_AGC,
 ST_TUNER_GAIN_MODE,
 ST_TUNER_GAIN,
 ST_TEST,
 ST_invalid
} rtl2832_cmd_t;


typedef struct dp_radio2832_sub_t {
  dp_conc_ringbuffer_t rb;
  rtlsdr_dev_t *dev;
  dp_bool_t dev_is_open;
  dp_bool_t running_async;     /* true of async thread still running */
  dp_bool_t report_overflow;   /* true if we want to hear about write 
				  overflows */
  uint32_t ec;                 /* accumulated error codes, if any */
  uint32_t curr_freq;
  int *gainslist;
  int gainscount;
} dp_radio2832_sub_t;


DP_SUB_CREATOR_DECL(radio2832);
DP_FN_DECL(radio2832,init);
DP_FN_DECL(radio2832,deinit);
DP_FN_DECL(radio2832,prerun);
DP_FN_DECL(radio2832,work);
DP_FN_DECL(radio2832,postrun);
pthread_t *dp_radio2832_start_thread(dp_base_t *);

int dp_radio2832_get_tuner_gains(dp_base_t *b);

DP_SUB_GENERIC_SETTER_DECL(radio2832,dev,rtlsdr_dev_t *);
int dp_radio2832_open_device(dp_base_t *, uint32_t index);
int dp_radio2832_close_device(dp_base_t *);
int dp_radio2832_list_devices(dp_base_t *);
void dp_radio2832_set_buffers(dp_base_t *b, uint32_t blen, uint32_t bcount);
DP_FN_DECL(radio2832,stop_async);
dp_bool_t *dp_radio2832_get_running_async(dp_base_t *b);
rtlsdr_dev_t *dp_radio2832_get_dev(dp_base_t *b);
DP_SUB_GENERIC_GETTER_DECL(radio2832,curr_freq,uint32_t);
DP_SUB_GENERIC_GETTER_DECL(radio2832,report_overflow,dp_bool_t);
dp_conc_ringbuffer_t *dp_radio2832_get_rb(dp_base_t *b);

int dp_radio2832_dev_cmd_st_only(dp_base_t *b, rtl2832_cmd_t c, uint32_t a);
int dp_radio2832_dev_cmd(dp_base_t *b, rtl2832_cmd_t c, uint32_t *a);

/* function to call to start the second thread */
void *rtl2832_thread_fn(void *f);

#endif


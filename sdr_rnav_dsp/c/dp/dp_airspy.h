
/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Fail 2015 
//
// Copyright 2015, David Jacobowitz
*/

#ifndef _DP_RADIO_AIRSPY_H
#define _DP_RADIO_AIRSPY_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "dp_conc_ringbuffer.h"
#include "dp_base_internals.h"

#include <libairspy/airspy.h>

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
typedef enum airspy_cmd_t {
 AS_ST_FREQ = 1,
 AS_ST_SR,
 AS_ST_VGA_GAIN,
 AS_ST_LNA_GAIN,
 AS_ST_MIX_GAIN,
 AS_ST_LNA_AGC,
 AS_ST_MIX_AGC,
 AS_ST_invalid
} airspy_cmd_t;


typedef struct dp_airspy_sub_t {
  dp_conc_ringbuffer_t rb;
  struct airspy_device *dev;
  dp_bool_t dev_is_open;
  dp_bool_t running_async;     /* true of async thread still running */
  dp_bool_t report_overflow;   /* true if we want to hear about write 
				  overflows */
  uint32_t ec;                 /* accumulated error codes, if any */
  uint32_t curr_freq;
  uint32_t *srlist;
  uint32_t srcount;
} dp_airspy_sub_t;


DP_SUB_CREATOR_DECL(airspy);
DP_FN_DECL(airspy,init);
DP_FN_DECL(airspy,deinit);
DP_FN_DECL(airspy,prerun);
DP_FN_DECL(airspy,work);
DP_FN_DECL(airspy,postrun);
pthread_t *dp_airspy_start_thread(dp_base_t *);

int dp_airspy_get_samplerates(dp_base_t *b);

DP_SUB_GENERIC_SETTER_DECL(airspy,dev,struct airspy_device *);
int dp_airspy_open_device(dp_base_t *, uint32_t index);
int dp_airspy_close_device(dp_base_t *);
void dp_airspy_set_buffers(dp_base_t *b, uint32_t blen, uint32_t bcount);
DP_FN_DECL(airspy,stop_async);
dp_bool_t *dp_airspy_get_running_async(dp_base_t *b);
struct airspy_device *dp_airspy_get_dev(dp_base_t *b);
DP_SUB_GENERIC_GETTER_DECL(airspy,curr_freq,uint32_t);
DP_SUB_GENERIC_GETTER_DECL(airspy,report_overflow,dp_bool_t);
dp_conc_ringbuffer_t *dp_airspy_get_rb(dp_base_t *b);

int dp_airspy_dev_cmd_st_only(dp_base_t *b, airspy_cmd_t c, uint32_t a);
int dp_airspy_dev_cmd(dp_base_t *b, airspy_cmd_t c, uint32_t *a);

/* function to call to start the second thread */
void *airspy_thread_fn(void *f);

#endif


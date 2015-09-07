
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef _RTL_HELP_H
#define _RTL_HELP_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <vector>
// #include <pthread.h>
#include "conc_ringbuffer.h"

#include <rtl-sdr.h>

#include "block_base.h"

namespace djdsp {

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


// Various command types to be sent to the RTL device
typedef enum rtl_cmd_t {
 ST_FREQ = 1,
 ST_SR,
 ST_AGC,
 ST_GAIN_MANUAL,
 ST_GAIN,
 ST_TEST,
 ST_invalid,
} rtl_cmd_t;

// a type for the ring buffer
typedef std::vector<dvec_t> dev_bufs_t;



class rtl_help_c : public block_base_c {
 public:
  rtl_help_c();
  ~rtl_help_c();
  int dev_cmd(rtl_cmd_t c, uint32_t a);
  void pre_run();
  void post_run();
  void work();
  // call only one of these two: either open the device yourself and 
  // pass it in or use the open command
  void set_dev(rtlsdr_dev_t *id);
  int open_rtl_device(uint32_t index);

  int close_rtl_device();
  int list_rtl_devices();
  
  // pass in the address of the object that will be consuming 
  // this data
  void set_consumer(block_base_c *c);
  // sets up the size and number of buffers for the ringbuffer
  void set_buffers(uint32_t blen, uint32_t bcount);
  // tell the other thread we don't need it anymore and it 
  // should exit with grace
  void stop_async();


  // All these getters are for the callback so that it 
  // can access what it needs to of the ring
  // buffer and mutexes. I think I can probably figure
  // out a way to get rid of these by passing the object
  // context ("this") back to the callback through its 
  // void * context pointer and then rebuilding the object
  // but maybe this is more sane, if ugly.
  concurrent_ringbuffer_c *_get_rb();

  bool *_get_running_async();
  rtlsdr_dev_t *_get_dev();
  bool _get_report_overflow();
  uint32_t getCurrFreq();

 private:
  // ringbuffer and its mutexes
  concurrent_ringbuffer_c rb;

  rtlsdr_dev_t *dev;
  bool dev_is_open;
  bool running_async;     // true of async thread still running
  bool report_overflow;   // true if we want to hear about write overflows
  block_base_c *consumer; // address of consuming block
  uint32_t ec;            // accumulated error codes, if any
  uint32_t curr_freq;
};

} // namespace

extern "C" {
// function to call to start the second thread
void *rtl_thread_fn(void *f);
}

#endif


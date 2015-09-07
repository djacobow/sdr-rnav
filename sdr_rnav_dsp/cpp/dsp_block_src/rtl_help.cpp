

// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz


#include "rtl_help.h"
#include <iostream>
#include <string.h>
// #include "my_console.h"

#ifdef __linux
#include <unistd.h>
void rtl_sleep(int ims) {
 usleep(ims*1000);
};
#else
#ifdef __WIN32
#include <windows.h>
void rtl_sleep(int ims) {
 Sleep(ims);
}
#endif
#endif

// Implementation of wrapper class for dealing with the RTL2832
// radio dongle, as provided by the rtl-sdr library.
// http://sdr.osmocom.org/trac/wiki/rtl-sdr

namespace djdsp {

rtl_help_c::rtl_help_c() {
 // std::cout << "rtl_help create" << std::endl;
 name = std::string("rtl_help_c");
 dev_is_open = false;
 running_async = false;
 dev = 0;
 ec = 0;
 report_overflow = false;
 consumer = 0;
}

rtl_help_c::~rtl_help_c() {
 if (running_async) {
  rtlsdr_cancel_async(dev); 
 }
 if (dev_is_open) {
  int close_rtl_device();
 };
};


void
rtl_help_c::set_dev(rtlsdr_dev_t *id) { 
 dev = id; 
 dev_is_open = true;
};

// note on buffer length, blen. It is the length of the buffer in 
// time samples. Each of those has an I and Q component, so the 
// length of the actual buffer with be 2x as long.
void
rtl_help_c::set_buffers(uint32_t blen,
		uint32_t bcount) {
 std::cout << "rtl_help (" << name << ") creating " 
           << bcount << " deep ringbuffer of size " << blen
           << std::endl;
 rb.set_buffers(bcount,blen*2);
 l = blen;
 l_set = true;
};

int
rtl_help_c::close_rtl_device() {
 int rv = 0;
 if (dev_is_open) {
  rv = rtlsdr_close(dev);
  dev_is_open = false;
 };
 return rv;
};

int
rtl_help_c::list_rtl_devices() {
 // std::cout << "rtl_help list devices" << std::endl;
 int dev_count = rtlsdr_get_device_count();
 if (dev_count >= 0) {
  for (int i=0;i<dev_count;i++) {
   char tbuf[50];
   memset(tbuf,0,50);
   strncpy(tbuf,rtlsdr_get_device_name(i),49);
   std::cout << "-info- (" << name << ") ";
   std::cout << "device " << i << " name ";
   std::cout << tbuf << std::endl;
  }
 } else {
  std::cerr << "-err-- rtlsdr: no devices?" << std::endl;
  ec |= 0x2;
  return 0;
 }
 return dev_count;
}

int
rtl_help_c::open_rtl_device(uint32_t index) {
 // std::cout << "rtl_help open device" << std::endl;
 int rv = rtlsdr_open(&dev,index); 
 if (rv) {
  std::cerr << "-err-- rtlsdr: could not open dev " << index 
            << " return code " << rv << std::endl;
 };
 dev_is_open = true;
 return rv; 
};


void
rtl_help_c::stop_async() {
 if (dev_is_open && running_async) {
  rtlsdr_cancel_async(dev);
  running_async = false;
 };
};

void
rtl_help_c::post_run() {
 stop_async();
};


void
rtl_help_c::pre_run() {
 report_overflow = true;
};

// These getters are a kluge to let the non-objecty pthreads routines
// have access to data in this object.
concurrent_ringbuffer_c *rtl_help_c::_get_rb()      { return &rb; };
rtlsdr_dev_t *rtl_help_c::_get_dev()                { return dev; };
bool *rtl_help_c::_get_running_async()              { return &running_async; };
bool  rtl_help_c::_get_report_overflow()            { return report_overflow; };
uint32_t rtl_help_c::getCurrFreq()                  { return curr_freq; };

void rb_callback(unsigned char *ibuf, uint32_t len, void *f) {
 // std::cout << "rtl_help rb callback" << std::endl;
 rtl_help_c *rhelp = (rtl_help_c *)f;
 concurrent_ringbuffer_c *prb = rhelp->_get_rb();
 bool overflow = prb->store_buffer8b(ibuf, len);
 if (overflow && rhelp->_get_report_overflow()) { 
   std::cout << "-warn- (rtlsdr) overflow (unread samples overwritten)"
             << std::endl;;
 } 
};



int
rtl_help_c::dev_cmd(rtl_cmd_t c, uint32_t a) {
 int rv = 0;
 switch (c) {
  case ST_FREQ :
   std::cout << "-info- (rtlsdr) changing freq to " << a << std::endl;
   rv = rtlsdr_set_center_freq(dev,a);
   curr_freq = a;
   break;
  case ST_SR :
   std::cout << "-info- (rtlsdr) changing sample rate to " << a << std::endl;
   rv = rtlsdr_set_sample_rate(dev,a);
   if (rv) {
    std::cout << "-warn- (rtlsdr) setting sample rate returned non-zero " << rv << std::endl;
   } 
   rv = rtlsdr_get_sample_rate(dev);
   break;
  case ST_AGC :
   std::cout << "-info- (rtlsdr) changing agc mode to " << a << std::endl;
   rv = rtlsdr_set_agc_mode(dev,a);
   break;
  case ST_GAIN_MANUAL:
   std::cout << "-info- (rtlsdr) changing manual gain mode to " << a << std::endl;
   rv = rtlsdr_set_tuner_gain_mode(dev,a);
   break;
  case ST_GAIN:
   std::cout << "-info- (rtlsdtr) changing gain to " << a << std::endl;
   rv = rtlsdr_set_tuner_gain(dev,a);
   break;
  case ST_TEST:
   std::cout << "-info- (rtlsdr) changing test mode to " << a << std::endl;
   rv = rtlsdr_set_testmode(dev,a);
   break;
  default:
   break;
 } 
 if (rv && (c != ST_SR)) {
  std::cerr << "-err-- rtlsdr: dev_cmd failed cmd " << (int)c 
            << " rv " << rv << std::endl;
 };
 return rv;
};

void rtl_help_c::set_consumer(block_base_c *c) { consumer = c; };

void rtl_help_c::work() {
 dvec_t *readbuffer = rb.get_curr_readbuffer(true);
 if (consumer) {
  consumer->set_in(readbuffer);
 } else {
  for (uint32_t i=0;i<l;i++) {
   (*out)[2*i]   = (*readbuffer)[2*i];
   (*out)[2*i+1] = (*readbuffer)[2*i+1];
  }
 }
};

} // namespace

extern "C" {

void *rtl_thread_fn(void *f) {
 // std::cout << "rtl_help thread fn" << std::endl;
 djdsp::rtl_help_c *rhelp = (djdsp::rtl_help_c *)f;
 djdsp::concurrent_ringbuffer_c *prb = rhelp->_get_rb();

 // the ringbuffer's buffers are allocated in the dsp thread, not this 
 // one, so we have to wait until that thread has gotten around to 
 // allocating those buffers, otherwise the calls to the ringbuffer
 // made by the callback specified below will generate exceptions.
 while (!prb->buffers_set()) {
  rtl_sleep(100);
 }

 int rv = rtlsdr_reset_buffer(rhelp->_get_dev());
 if (rv) {
  std::cerr << "-err-- (rtlsdr) could not reset rtl buffer (" << rv 
            << ")" << std::endl;
 } else {
  *(rhelp->_get_running_async()) = true;
  rv = rtlsdr_read_async(rhelp->_get_dev(), &djdsp::rb_callback, f, prb->get_buff_count(),
		         prb->get_buff_len());

  if (rv) {
   std::cerr << "-err-- (rtlsdt) could not start read_async callback routine"
             << std::endl;
  } else {
   std::cout << "-info- (rtlsdr) async buffer filling stopped" << std::endl;
  }
 }
 return 0;
};

}

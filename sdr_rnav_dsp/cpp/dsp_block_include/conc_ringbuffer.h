

// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef __CONC_RINGBUFFER_H
#define __CONC_RINGBUFFER_H

#include <pthread.h>
#include <vector>
#include "block_base.h"

namespace djdsp {

typedef std::vector<dvec_t> dev_bufs_t;

class concurrent_ringbuffer_c {

 public:
  concurrent_ringbuffer_c();
  ~concurrent_ringbuffer_c();
  bool store_buffer8b(const unsigned char *ivec, uint32_t len);
  bool read_buffer(dvec_t &ovec, bool block);
  dvec_t *get_curr_readbuffer(bool block);
  void set_buffers(unsigned int count, unsigned int length);
  uint32_t get_buff_count(); // number of buffers in ring
  uint32_t get_buff_len(); // length in elements
  bool buffers_set(void); // true if set_buffers has been called
 private:
  pthread_mutex_t rb_mutex;
  pthread_cond_t  rb_cond;
  dev_bufs_t      rbs;
  int             reader_unread;
  int             writer_next;
  bool            buffers_are_set;
};



} // namespace


#endif


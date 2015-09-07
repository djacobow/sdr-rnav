

// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#include "conc_ringbuffer.h"
#include <iostream>

namespace djdsp {

// Simple ringbuffer class, with the complication that writing to the buffer 
// and reading from it are multi-thread friendly with pthread mutexes.
//
// This one has some slightly strange semantics. 
//  1 writes are non blocking with respect to fullness. If the buffer is full, 
//    then just overwrite some old data
//  2 writes will still block if a read is actually going on 
//  3 reads can block or not, depending on a bool set by the caller. If they 
//    are blocking, then when the funciton returns, you have a good buffer
//    If they are non-blocking, the retun value tells you if your buffer has
//    data.

concurrent_ringbuffer_c::concurrent_ringbuffer_c() {
 pthread_mutex_init(&rb_mutex,0);
 pthread_cond_init(&rb_cond,0);
 reader_unread = 0;
 writer_next = 0;
 buffers_are_set = false;
};

concurrent_ringbuffer_c::~concurrent_ringbuffer_c() {
 pthread_mutex_destroy(&rb_mutex);
 pthread_cond_destroy(&rb_cond);
};

void
concurrent_ringbuffer_c::set_buffers(unsigned int count, unsigned int length) {
 rbs.resize(count);
 for (unsigned int i=0;i<count;i++) {
  rbs.at(i).resize(length);
 }
 buffers_are_set = true;
}

bool
concurrent_ringbuffer_c::buffers_set(void) {
 return buffers_are_set;
}

uint32_t 
concurrent_ringbuffer_c::get_buff_count() {
 return rbs.size();
}

uint32_t 
concurrent_ringbuffer_c::get_buff_len() {
 return rbs.at(0).size();
};

bool
concurrent_ringbuffer_c::store_buffer8b(const unsigned char *ivec, uint32_t len) {
 bool overflow = false;
 pthread_mutex_lock(&rb_mutex);
 int16_t *obuf = rbs.at(writer_next).data();
 uint32_t size = rbs.at(writer_next).size();
 if (size != len) {
  std::cerr << "-warn- block size and length do not match!" 
	    << std::endl;
  if (len < size) {
   size = len;
  }
 }

 // note, depending on the order of samples comoing from 
 // the dongle, we may want to reverse IQ
 for (uint32_t i=0;i<size;i++) {
  int8_t idatum = ivec[i];
  int16_t odatum = 255 * idatum;
  obuf[i] = odatum;
 }

 int rblen = rbs.size();
 ++writer_next;
 if (writer_next == rblen) { writer_next = 0; }
 if (reader_unread < rblen) {
  ++reader_unread;
 } else {
  overflow = true;
 }
 pthread_cond_signal(&rb_cond);
 pthread_mutex_unlock(&rb_mutex);
 return overflow;
};

// This routine gets the current read buffer but only locks
// while adjusting the pointer. The readbuffer you get can 
// still be clobbered while you're using it. Using this function
// is probably not a good idea.
dvec_t *
concurrent_ringbuffer_c::get_curr_readbuffer(bool block) {
 pthread_mutex_lock(&rb_mutex);
 dvec_t *rv = 0;
 if (reader_unread == 0) {
  if (block) {
   pthread_cond_wait(&rb_cond, &rb_mutex);
  } else {
   pthread_mutex_unlock(&rb_mutex);
   return rv;
  }
 }
 if (reader_unread > 0) {
  int pos = writer_next - reader_unread;
  if (pos < 0) { pos += rbs.size(); };
  rv = &(rbs.at(pos));
  --reader_unread;
 } else {
  std::cerr << "-crb- we should not be here!" << std::endl;
 }
 pthread_mutex_unlock(&rb_mutex);
 return rv;
};



bool
concurrent_ringbuffer_c::read_buffer(dvec_t &ovec, bool block) {
 pthread_mutex_lock(&rb_mutex);
 if (reader_unread == 0) {
  if (!block) {
   pthread_mutex_unlock(&rb_mutex);
   return true;
  }
  pthread_cond_wait(&rb_cond, &rb_mutex);
 }
 if (reader_unread > 0) {
  int pos = writer_next - reader_unread;
  if (pos < 0) { pos += rbs.size(); };

  int16_t *rbd = rbs.at(pos).data();
  for (unsigned int i=0;i<ovec.size();i++) {
   ovec[i] = rbd[i];
  }
  --reader_unread;
 }
 pthread_mutex_unlock(&rb_mutex);
 return false;
};

} // namespace

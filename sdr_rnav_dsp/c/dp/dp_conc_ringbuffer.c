
/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#include "dp_conc_ringbuffer.h"
#include <stdio.h>

/*
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
*/

void
dp_conc_ringbuffer_init(dp_conc_ringbuffer_t *b) {
 if (b) {
  pthread_mutex_init(&b->rb_mutex,0);
  pthread_cond_init(&b->rb_cond,0);
  b->reader_unread = 0;
  b->writer_next = 0;
  b->buffers_are_set = 0;
  b->buffer_ct = 0;
 }
}

void
dp_conc_ringbuffer_deinit(dp_conc_ringbuffer_t *b) {
 uint32_t i;
 pthread_mutex_destroy(&b->rb_mutex);
 pthread_cond_destroy(&b->rb_cond);
 if (b->buffers_are_set) {
  for (i=0;i<b->buffer_ct;b++) {
   dp_vec_deinit(b->rbs[i]);
   free(b->rbs[i]);
  }
  free(b->rbs);
 }
}

void
dp_conc_ringbuffer_set_buffers(dp_conc_ringbuffer_t *b, unsigned int count,
		unsigned int length) {
 uint32_t i;
 b->rbs = malloc(sizeof(dp_vec_t *) * count);
 if (b->rbs) {
  for (i=0;i<count;i++) {
   b->rbs[i] = malloc(sizeof(dp_vec_t) * length);
   dp_vec_init(b->rbs[i], length);
  }
  b->buffers_are_set = 1;
  b->buffer_ct = count;
 }
}

dp_bool_t
dp_conc_ringbuffer_buffers_set(dp_conc_ringbuffer_t *b) {
 return b->buffers_are_set;
}

uint32_t 
dp_conc_ringbuffer_get_buff_count(dp_conc_ringbuffer_t *b) {
 return b->buffer_ct;
}

uint32_t 
dp_conc_ringbuffer_get_buff_len(dp_conc_ringbuffer_t *b) {
 if (b && b->rbs && b->rbs) {
  return b->rbs[0]->len;
 }
 return 0;
}

dp_bool_t
dp_conc_ringbuffer_store_buffer16b(dp_conc_ringbuffer_t *b,
                                   const int16_t *ivec,
                                   uint32_t len) {
 int16_t *obuf;
 uint32_t size;
 uint32_t i;
 int rblen;
 dp_bool_t overflow = 0;
 pthread_mutex_lock(&b->rb_mutex);
 obuf = b->rbs[b->writer_next]->v;
 size = b->rbs[b->writer_next]->len;
 if (size != len) {
  fprintf(stderr,"-warn- ringbuffer_store block size and length do not match!\n" );
  if (len < size) {
   size = len;
  }
 }
 for (i=0;i<size;i++) {
  obuf[i] = ivec[i];
 }

 rblen = b->buffer_ct;
 ++b->writer_next;
 if (b->writer_next == rblen) { b->writer_next = 0; }
 if (b->reader_unread < rblen) {
  ++b->reader_unread;
 } else {
  overflow = 1;
 }
 pthread_cond_signal(&b->rb_cond);
 pthread_mutex_unlock(&b->rb_mutex);
 return overflow;
}

dp_bool_t 
dp_conc_ringbuffer_store_buffer8b(dp_conc_ringbuffer_t *b,
		                  const unsigned char *ivec, 
				  uint32_t len) {
 int16_t *obuf;
 uint32_t size;
 uint32_t i;
 int rblen;
 dp_bool_t overflow = 0;
 pthread_mutex_lock(&b->rb_mutex);
 obuf = b->rbs[b->writer_next]->v;
 size = b->rbs[b->writer_next]->len;
 if (size != len) {
  fprintf(stderr,"-warn- ringbuffer_store block size and length do not match!\n" );
  if (len < size) {
   size = len;
  }
 }
 /* note, depending on the order of samples comoing from 
 // the dongle, we may want to reverse IQ */
 for (i=0;i<size;i++) {
  int8_t idatum = ivec[i] - 127;
  int16_t odatum = idatum << 8;
  obuf[i] = odatum;
 }

 rblen = b->buffer_ct;
 ++b->writer_next;
 if (b->writer_next == rblen) { b->writer_next = 0; }
 if (b->reader_unread < rblen) {
  ++b->reader_unread;
 } else {
  overflow = 1;
 }
 pthread_cond_signal(&b->rb_cond);
 pthread_mutex_unlock(&b->rb_mutex);
 return overflow;
}

/*
// This routine gets the current read buffer but only locks
// while adjusting the pointer. The readbuffer you get can 
// still be clobbered while you're using it. Using this function
// is probably not a good idea.
*/
dp_vec_t *
dp_conc_ringbuffer_get_curr_readbuffer(dp_conc_ringbuffer_t *b, dp_bool_t block) {
 dp_vec_t *rv = 0;
 pthread_mutex_lock(&b->rb_mutex);
 if (b->reader_unread == 0) {
  if (block) {
   pthread_cond_wait(&b->rb_cond, &b->rb_mutex);
  } else {
   pthread_mutex_unlock(&b->rb_mutex);
   return rv;
  }
 }
 if (b->reader_unread > 0) {
  int pos = b->writer_next - b->reader_unread;
  if (pos < 0) { pos += b->buffer_ct; }
  rv = b->rbs[pos];
  --b->reader_unread;
 } else {
  fprintf(stderr, "-ce- we should not be here!\n");
 }
 pthread_mutex_unlock(&b->rb_mutex);
 return rv;
}



dp_bool_t 
dp_conc_ringbuffer_read_buffer(dp_conc_ringbuffer_t *b, dp_vec_t *ovec, dp_bool_t block) {
 pthread_mutex_lock(&b->rb_mutex);
 if (b->reader_unread == 0) {
  if (!block) {
   pthread_mutex_unlock(&b->rb_mutex);
   return 1;
  }
  pthread_cond_wait(&b->rb_cond, &b->rb_mutex);
 }
 if (b->reader_unread > 0) {
  uint32_t i;
  dp_vec_t *rbd;
  int pos = b->writer_next - b->reader_unread;
  if (pos < 0) { pos += b->buffer_ct; }

  rbd = b->rbs[pos];
  for (i=0;i<ovec->len;i++) {
   ovec->v[i] = rbd->v[i];
  }
  --b->reader_unread;
 }
 pthread_mutex_unlock(&b->rb_mutex);
 return 0;
}

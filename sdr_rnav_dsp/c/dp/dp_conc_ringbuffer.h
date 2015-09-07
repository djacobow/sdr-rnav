

/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifndef __DP_CONC_RINGBUFFER_H
#define __DP_CONC_RINGBUFFER_H

#include <pthread.h>
#include "dp_base.h"
#include "dp_vector.h"

typedef struct dp_conc_ringbuffer_t {
 dp_vec_t **rbs;
 uint32_t buffer_ct;
 int      reader_unread;
 int      writer_next;
 dp_bool_t buffers_are_set;
 pthread_mutex_t rb_mutex;
 pthread_cond_t  rb_cond;
} dp_conc_ringbuffer_t;

void
dp_conc_ringbuffer_init(dp_conc_ringbuffer_t *b);
void
dp_conc_ringbuffer_deinit(dp_conc_ringbuffer_t *b);

dp_bool_t 
dp_conc_ringbuffer_store_buffer8b(dp_conc_ringbuffer_t *b,
		const unsigned char *ivec, uint32_t len);

dp_bool_t 
dp_conc_ringbuffer_store_buffer16b(dp_conc_ringbuffer_t *b,
		const int16_t *ivec, uint32_t len);

dp_bool_t 
dp_conc_ringbuffer_read_buffer(dp_conc_ringbuffer_t *b,
		               dp_vec_t *ovec, dp_bool_t block);

dp_vec_t *
dp_conc_ringbuffer_get_curr_readbuffer(dp_conc_ringbuffer_t *b,
		                       dp_bool_t block);

void
dp_conc_ringbuffer_set_buffers(dp_conc_ringbuffer_t *b, 
		               unsigned int count, 
			       unsigned int length);

uint32_t 
dp_conc_ringbuffer_get_buff_count(dp_conc_ringbuffer_t *b);

uint32_t 
dp_conc_ringbuffer_get_buff_len(dp_conc_ringbuffer_t *b);

dp_bool_t
dp_conc_ringbuffer_buffers_set(dp_conc_ringbuffer_t *b);

#endif


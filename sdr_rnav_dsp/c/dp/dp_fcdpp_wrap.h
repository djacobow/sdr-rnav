
/*
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2014
//
// Copyright 2014, David Jacobowitz
*/

#ifndef _DP_FCDPP_WRAP_H
#define _DP_FCDPP_WRAP_H

#include <inttypes.h>

#ifdef __linux
 #include <alsa/asoundlib.h>
#endif

#ifdef _WIN32
 /* windows stuff */
#endif

#include "dp_base.h"

typedef void(*fcdpp_wrapper_async_cb_t)(int16_t *buf, uint32_t len, void *ctx);

typedef struct fcdpp_wrapper_t {
#ifdef __linux
 snd_pcm_t *handle;
 snd_pcm_hw_params_t *hw_params;
#endif
#ifdef _WIN32
 /* windows stuff */
#endif
 char *devname;
 dp_bool_t is_running;
 dp_bool_t is_open;
 fcdpp_wrapper_async_cb_t callback;
 int16_t **buffers;
 uint16_t buf_count;
 uint32_t buf_len;
 uint32_t buf_num;
} fcdpp_wrapper_t;


void     fcdpp_wrapper_cancel_async(fcdpp_wrapper_t *d);
int      fcdpp_wrapper_close(fcdpp_wrapper_t *d);
int      fcdpp_wrapper_open(fcdpp_wrapper_t **d, char *name);
uint32_t fcdpp_wrapper_set_frequency(fcdpp_wrapper_t *d, uint32_t in_f);
int      fcdpp_wrapper_reset_buffer(fcdpp_wrapper_t *d);


int      fcdpp_wrapper_read_async(fcdpp_wrapper_t *d, fcdpp_wrapper_async_cb_t cb, void *ctx, uint32_t buf_num, uint32_t buf_len);

#endif


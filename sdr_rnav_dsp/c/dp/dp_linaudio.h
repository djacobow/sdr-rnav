
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#ifdef __linux

#ifndef _DP_LINUX_AUDIO_H
#define _DP_LINUX_AUDIO_H

#include "dp_base_internals.h"

#include <stdlib.h>
#include <stdio.h>
#include <alsa/asoundlib.h>

DP_SUB_CREATOR_DECL(linaudio);
DP_FN_DECL(linaudio,init);
DP_FN_DECL(linaudio,deinit);
DP_FN_DECL(linaudio,prerun);
DP_FN_DECL(linaudio,work);
DP_FN_DECL(linaudio,reset);
int dp_linaudio_setup(dp_base_t *b);
DP_FN_DECL(linaudio,unsetup);
DP_SUB_GENERIC_SETTER_DECL(linaudio,sample_rate,uint32_t);
DP_SUB_GENERIC_SETTER_DECL(linaudio,num_channels,uint8_t);
DP_SUB_GENERIC_SETTER_DECL(linaudio,bits_per_sample,uint8_t);
DP_SUB_STRING_SETTER_DECL(linaudio,device_name);

typedef struct dp_linaudio_sub_t {
  dp_bool_t isopen;
  uint8_t  bits_per_sample;
  uint32_t sample_rate;
  uint16_t num_channels;
  uint32_t bytes_written;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  snd_pcm_uframes_t frames;
  char *device_name;
  int dir;
  unsigned int buf_time;
  unsigned int buf_len;
  snd_pcm_status_t *stat;
  unsigned int iter;
} dp_linaudio_sub_t;

#endif

#endif


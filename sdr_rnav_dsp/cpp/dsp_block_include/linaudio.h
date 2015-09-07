
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifdef __linux

#ifndef _LINUX_AUDIO_H
#define _LINUX_AUDIO_H

#include "block_base.h"

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <alsa/asoundlib.h>

namespace djdsp {

class linux_audio_c : public block_base_c {

 public:
  linux_audio_c();
  ~linux_audio_c();
  void set_sample_rate(uint32_t);
  void set_num_channels(uint8_t);
  void set_bits_per_sample(uint8_t);
  void set_device_name(std::string);
  void pre_run();
  void work();
  void reset();
 private:
  int setup();
  void unsetup();

  bool isopen;
  uint8_t  bits_per_sample;
  uint32_t sample_rate;
  uint16_t num_channels;
  uint32_t bytes_written;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  snd_pcm_uframes_t frames;
  std::string devname;
  int dir;
  unsigned int buf_time;
  unsigned int buf_len;
  snd_pcm_status_t *stat;
  // bool first;
  unsigned int iter;
};

} // namespace

#endif

#endif


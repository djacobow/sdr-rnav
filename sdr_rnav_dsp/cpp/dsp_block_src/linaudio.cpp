
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz


#ifdef __linux

#include "linaudio.h"
#include <iostream>
#include <string>

namespace djdsp {

// Implmentation of DSP block class that writes data to the audio output.
// Works on Linux only using the ALSA library.

linux_audio_c::linux_audio_c() {
 isopen = false;
 devname = "default";
 num_channels = 1;
 bits_per_sample = 16;
 buf_time = 0;
 buf_len = 0;
 iter = 0;
};

void
linux_audio_c::set_sample_rate(uint32_t isr) {
 sample_rate = isr;
}

void
linux_audio_c::set_num_channels(uint8_t inc) {
 num_channels = inc;
}

void
linux_audio_c::set_bits_per_sample(uint8_t ibps) {
 bits_per_sample = ibps;
}


void
linux_audio_c::set_device_name(std::string in) {
 devname = in;
}

void
linux_audio_c::pre_run() {
 if (!isopen) {
  setup();
 }
}


int
linux_audio_c::setup() {

 std::cout << "-info- (" << name << ") setting up audio pcm" << std::endl; 

 if (isopen) {
  unsetup();
 }
 if (!isopen) {
  int rc = snd_pcm_open(&handle, devname.c_str(), SND_PCM_STREAM_PLAYBACK,0);
  if (rc < 0) {
   std::cout << "-error- (" << name << ") could not open pcm device" 
             << std::endl
             << snd_strerror(rc) << std::endl;
   return -1;
  }
  snd_pcm_hw_params_alloca(&params);
  snd_pcm_hw_params_any(handle, params); 
  snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(handle, params, 
             bits_per_sample == 16 ? SND_PCM_FORMAT_S16_LE : 
             bits_per_sample == 8  ? SND_PCM_FORMAT_S8 : 
             SND_PCM_FORMAT_UNKNOWN);
  snd_pcm_hw_params_set_channels(handle,params,num_channels);
  snd_pcm_hw_params_set_rate_near(handle,params,&sample_rate,&dir);
  frames = l;
  snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
   std::cout << "-error- (" << name 
             << ") could not set pcm hardware parameters" << std::endl
             << snd_strerror(rc) << std::endl;
   return -1;
  }
  snd_pcm_hw_params_get_period_size(params, &frames, &dir);
  // snd_pcm_hw_params_get_period_time(params, &buf_time, &dir);
  buf_len = frames;
  isopen = true;
  // first = true;
  iter = 0;


  // The ALSA library is quirky and flaky as near as I can tell, and it just works
  // better if I give it some time to cool off before starting to send it samples 
  // in the work() function. I don't know how or why this works, but it somehow 
  // avoid buffer underruns which really muck things up because it takes ALSA a 
  // surprisingly long time to recover from an underrun. I tried sending a few 
  // empty buffers to keep the ALSA busy before work() started getting called but
  // that doesn't work.
  if (0) {
   std::cout << "frame length is " << frames << std::endl;
   unsigned short *d = new unsigned short[frames];
   for (unsigned int i=0;i<frames;i++) { d[i] = i % 300; };
   memset(d,0,buf_len * sizeof(unsigned short));
   snd_pcm_writei(handle, d, frames);
   snd_pcm_writei(handle, d, frames);
   snd_pcm_writei(handle, d, frames);
   snd_pcm_writei(handle, d, frames);
   snd_pcm_writei(handle, d, frames);
   snd_pcm_writei(handle, d, frames);
   snd_pcm_prepare(handle);
   snd_pcm_start(handle);
   delete d;
  } else {
   usleep(1000000);
  }
 } 
 return 0;
};

void
linux_audio_c::unsetup() {
 if (isopen) {
  snd_pcm_drain(handle);
  int rc = snd_pcm_close(handle);
  if (rc < 0) {
   std::cout << "-err- (" << name << ") problem closing pcm" << std::endl;
  }
  isopen = false;
 }
}

linux_audio_c::~linux_audio_c() {
 if (isopen) {
  unsetup();
 }
}

void
linux_audio_c::reset() {
 if (isopen) {
  unsetup();
 }
}

void
linux_audio_c::work() {

 // NB: This code is somewhat flakey, and I'm still not entirely certain
 // what it sometimes works and othertimes doesn't. What I do know is that 
 // sometimes the audio buffer underruns, I guess because it is being filled
 // too slowly. Because the radio is running in real-time, it's not hard to 
 // see how this happens, the samples are produced at the same rate they are 
 // consumed, so a glitch or a lag here or there and you can be in trouble. 
 // One might imagine that the way to manage that is to let a few buffers 
 // fill before running, but that didn't seem to work.
 //
 // Anyway, still experimenting!

 // if (iter == 0) {
 //  snd_pcm_prepare(handle);
 // }

 const int16_t *d = in->data();
 int rc = snd_pcm_writei(handle, d, frames);
 if (rc == -EPIPE) {
  std::cout << "-warn- (" << name << ") audio underrun, iter " << iter << std::endl; 
  // snd_pcm_prepare(handle);
  snd_pcm_recover(handle,rc, 1);
 } else if (rc < 0) {
  std::cout << "-err-  (" << name << ") from audio writei, iter" << iter << std::endl
            << snd_strerror(rc) << std::endl;
  snd_pcm_recover(handle,rc, 1);
 } else {
  if (rc != (int)frames) {
   std::cout << "-warn- (" << name << ") only wrote " << rc 
             << " of " << frames << " data, iter " << iter << std::endl; 
   snd_pcm_recover(handle,rc, 1);
  }
 };
 
 // snd_pcm_state_t s = snd_pcm_state(handle);
 // if (s != SND_PCM_STATE_RUNNING) {
 //  std::cout << "-warn- (" << name << ") pcm not in running state "
 //            << std::endl;
 // }

 // if (iter == 100) {
 //   snd_pcm_start(handle);
 // }

 iter++;
};

} // namespace

#endif



/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/


#ifdef __linux

#include "dp_linaudio.h"

/* Implmentation of DSP block class that writes data to the audio output.
// Works on Linux only using the ALSA library.
*/

static const char default_devname[] = "default";

DP_SUB_CREATOR_IMPL(linaudio)

DP_FN_PREAMBLE(linaudio,init) {
 s->isopen = dp_false;
 s->device_name = malloc(strlen(default_devname) + 1);
 if (s->device_name) {
  strcpy(s->device_name,default_devname);
 }
 s->num_channels = 1;
 s->bits_per_sample = 16;
 s->buf_time = 0;
 s->buf_len = 0;
 s->iter = 0;
 b->sub_work = &dp_linaudio_work;
 b->sub_prerun = &dp_linaudio_setup;
 b->sub_deinit = &dp_linaudio_deinit;
}
DP_FN_POSTAMBLE

DP_SUB_GENERIC_SETTER_IMPL(linaudio,sample_rate,uint32_t)
DP_SUB_GENERIC_SETTER_IMPL(linaudio,num_channels,uint8_t)
DP_SUB_GENERIC_SETTER_IMPL(linaudio,bits_per_sample,uint8_t)
DP_SUB_STRING_SETTER_IMPL(linaudio,device_name)

DP_FN_PREAMBLE(linaudio,prerun) {
 if (!s->isopen) {
  dp_linaudio_setup(b);
 }
}
DP_FN_POSTAMBLE


int
dp_linaudio_setup(dp_base_t *b) {
 DP_IFSUBVALID(b,linaudio) {
 fprintf(stderr,"-ci- (%s) setting up audio pcm\n",b->name);

 if (s->isopen) {
  dp_linaudio_unsetup(b);
 }
 if (!s->isopen) {
  int rc = snd_pcm_open(&s->handle, s->device_name, SND_PCM_STREAM_PLAYBACK,0);
  if (rc < 0) {
   fprintf(stderr, "-ce- (%s) could not open pcm device: %s\n",b->name,
		   snd_strerror(rc));
   return -1;
  }
  snd_pcm_hw_params_alloca(&s->params);
  snd_pcm_hw_params_any(s->handle, s->params); 
  snd_pcm_hw_params_set_access(s->handle, s->params, 
		  SND_PCM_ACCESS_RW_INTERLEAVED);
  snd_pcm_hw_params_set_format(s->handle, s->params, 
             s->bits_per_sample == 16 ? SND_PCM_FORMAT_S16_LE : 
             s->bits_per_sample == 8  ? SND_PCM_FORMAT_S8 : 
             SND_PCM_FORMAT_UNKNOWN);
  snd_pcm_hw_params_set_channels(s->handle,s->params,s->num_channels);
  snd_pcm_hw_params_set_rate_near(s->handle,s->params,&s->sample_rate,&s->dir);
  s->frames = b->runlength;
  snd_pcm_hw_params_set_period_size_near(s->handle, s->params, &s->frames, &s->dir);
  rc = snd_pcm_hw_params(s->handle, s->params);
  if (rc < 0) {
   fprintf(stderr,"-ce- (%s) could not set pcm hardware parameters (%s)",b->name,snd_strerror(rc));
   return -1;
  }
  snd_pcm_hw_params_get_period_size(s->params, &s->frames, &s->dir);
  s->buf_len = s->frames;
  s->isopen = dp_true;
  s->iter = 0;


  /* The ALSA library is quirky and flaky as near as I can tell, and it just works
  // better if I give it some time to cool off before starting to send it samples 
  // in the work() function. I don't know how or why this works, but it somehow 
  // avoid buffer underruns which really muck things up because it takes ALSA a 
  // surprisingly long time to recover from an underrun. I tried sending a few 
  // empty buffers to keep the ALSA busy before work() started getting called but
  // that doesn't work.
  */
  /* usleep(1000000); */
 } 
 }
 return 0;
}


DP_FN_PREAMBLE(linaudio,unsetup) {
 if (s->isopen) {
  snd_pcm_drain(s->handle);
  int rc = snd_pcm_close(s->handle);
  if (rc < 0) {
   fprintf(stderr,"-ce- (%s) problem closing pcm\n",b->name);
  }
  s->isopen = dp_false;
 }
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(linaudio,deinit) {
 if (s->isopen) {
  dp_linaudio_unsetup(b);
 }
 if (s->device_name) { free(s->device_name); }
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(linaudio,reset) {
 if (s->isopen) {
  dp_linaudio_unsetup(b);
 }
}
DP_FN_POSTAMBLE


DP_FN_PREAMBLE(linaudio,work) {

 /* NB: This code is somewhat flakey, and I'm still not entirely certain
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
 */

 const int16_t *d = b->in_v->v;
 int rc = snd_pcm_writei(s->handle, d, s->frames);
 if (rc == -EPIPE) {
  fprintf(stderr, "-cw- (%s) audio underrun, iter %d\n",b->name,s->iter);
  snd_pcm_recover(s->handle,rc, 1);
 } else if (rc < 0) {
  fprintf(stderr, "-ce-  (%s) from audio writei, iter %d\n",b->name,s->iter);
  fprintf(stderr, "-ce- returned: %s\n",snd_strerror(rc));
  snd_pcm_recover(s->handle,rc, 1);
 } else {
  if (rc != (int)s->frames) {
   fprintf(stderr,"-cw- (%s) only wrote %d of %d frames, iter %d\n",
		   b->name,rc,(int)s->frames,s->iter);
   snd_pcm_recover(s->handle,rc, 1);
  }
 }
 s->iter++;
}
DP_FN_POSTAMBLE

#else 
int _dp_lin_dummy; /* ansi c does not allow an empty file */
#endif


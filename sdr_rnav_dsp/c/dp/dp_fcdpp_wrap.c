
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2014
//
// Copyright 2014, David Jacobowitz
*/

#include "dp_fcdpp_wrap.h"

#ifdef __linux

void
fcdpp_wrapper_cancel_async(fcdpp_wrapper_t *d) {
 d->is_running = 0;
}


int fcdpp_wrapper_close(fcdpp_wrapper_t *d) {
 if (d->is_open) {
  snd_pcm_close(d->handle);
 }
 d->is_open = 0;
 return 0;
}

int fcdpp_wrapper_open(fcdpp_wrapper_t **d, char *name) {
 int rv;
 uint32_t rate = 192000;

 fcdpp_wrapper_t *dev = malloc(sizeof(fcdpp_wrapper_t));
 *d = dev;

 rv = snd_pcm_open(&dev->handle, name, SND_PCM_STREAM_CAPTURE, 0);
 if (rv<0) { puts(snd_strerror(rv)); };

 rv = snd_pcm_hw_params_malloc(&dev->hw_params);
 if (rv<0) { puts(snd_strerror(rv)); };
 rv = snd_pcm_hw_params_any(dev->handle,dev->hw_params);
 if (rv<0) { puts(snd_strerror(rv)); };
 rv = snd_pcm_hw_params_set_access(dev->handle,dev->hw_params,SND_PCM_ACCESS_RW_INTERLEAVED);
 if (rv<0) { puts(snd_strerror(rv)); };
 rv = snd_pcm_hw_params_set_format(dev->handle,dev->hw_params,SND_PCM_FORMAT_S16_LE);
 if (rv<0) { puts(snd_strerror(rv)); };
 rv = snd_pcm_hw_params_set_rate(dev->handle,dev->hw_params,rate,0);
 if (rv<0) { puts(snd_strerror(rv)); };
 rv = snd_pcm_hw_params_set_channels(dev->handle,dev->hw_params,2);
 if (rv<0) { puts(snd_strerror(rv)); };
 rv = snd_pcm_hw_params(dev->handle,dev->hw_params);
 if (rv<0) { puts(snd_strerror(rv)); };
 snd_pcm_hw_params_free(dev->hw_params);
 rv = snd_pcm_nonblock(dev->handle,0);
 if (rv<0) { puts(snd_strerror(rv)); };
 rv = snd_pcm_prepare(dev->handle); 
 if (rv<0) { puts(snd_strerror(rv)); };
 printf("hello there!\n");
 dev->is_open = 1;
 return rv;
}



uint32_t fcdpp_wrapper_set_frequency(fcdpp_wrapper_t *d, uint32_t in_f) {
 return in_f;
}


int fcdpp_wrapper_reset_buffer(fcdpp_wrapper_t *d) {
 /* do some stuff to reset internal buffers (if I have any) */
 return 0;
}


int fcdpp_wrapper_read_async(fcdpp_wrapper_t *d, 
                             fcdpp_wrapper_async_cb_t cb,
                             void *ctx,
                             uint32_t buf_num,
                             uint32_t buf_len) {  /* int16_t's*/

 /* setup */
 int i; 
 uint32_t count;
 int rv;
 void *context;
 uint32_t frames;

 snd_pcm_status_t *status;
 snd_pcm_status_malloc(&status);
 uint32_t frame_len   = buf_len / 2;

 float buffer_usec = (float)frame_len / 0.192;

 printf("-i- each radio buffer should take %f microseconds\n",buffer_usec);

 d->buf_len = buf_len;
 d->buf_num = buf_num; 
 context = ctx; 
 d->buffers = malloc(sizeof(int16_t *) * buf_num);
 for (i=0;i<buf_num;i++) {
  d->buffers[i] = malloc(sizeof(int16_t) * buf_len);
 }
 printf("buffer malloced\n");

 d->callback = cb;
 d->is_running = 1;

 int16_t *b = d->buffers[0];

 // startup craziness. Keep recovering and 
 // trying to read bytes until we start seeing 
 // the buffer start to fill. I don't understand
 // ALSA's behavior here, but this seems to get it
 // going semi-reliably.
 int running = false; 
 int error = false;
 while (!running && !error) {
  if (snd_pcm_status(d->handle,status)) {
   error = true;
  } else {
   if (!snd_pcm_status_get_avail(status)) {
    snd_pcm_recover(d->handle,0,0);
    rv = snd_pcm_readi(d->handle,b,frame_len);
   } else {
    printf("yay, fcd+alsa seem to have started!\n");
    running = true;
   }
  }
 }

 /* asynchronous loop */
 while (d->is_running) {
  rv = snd_pcm_status(d->handle, status);
  if (rv<0) {
   puts(strerror(rv));
   return rv;
  }  else {
   frames = snd_pcm_status_get_avail(status); 
   b = d->buffers[i %d->buf_num];
   if (frames < 0) {
    fprintf(stderr,"-err- get_avail: %s",strerror(frames));
   } else if (frames > frame_len) {
    rv = snd_pcm_readi(d->handle,b,frame_len);
    printf("-i- frames read %d\n",rv);
    if (rv<0) {
     puts(strerror(rv));
    } else {
     d->callback(b,rv*2,context);
    }
    printf("-i- callback made\n");
    count++;
   } else {
    usleep((int)(buffer_usec/4.0)); /* check every quarter buffer time */
   }
  } 
 }

 /* cleanup */
 for (i=0;i<d->buf_num;i++) {
  free(d->buffers[i]);
 } 
 free(d->buffers);
 free(status);
 return 0;
}

#endif

#ifdef _WIN32
 /* windows to come */

void
fcdpp_wrapper_cancel_async(fcdpp_wrapper_t *d) {
 assert(0);
}


int fcdpp_wrapper_close(fcdpp_wrapper_t *d) {
 assert(0);
 return -1;
}

int fcdpp_wrapper_open(fcdpp_wrapper_t **d, char *name) {
 assert(0);
 return -1;
}

uint32_t fcdpp_wrapper_set_frequency(fcdpp_wrapper_t *d, uint32_t in_f) {
 assert(0);
 return -1;
}


int fcdpp_wrapper_reset_buffer(fcdpp_wrapper_t *d) {
 assert(0);
 return -1;
}


int fcdpp_wrapper_read_async(fcdpp_wrapper_t *d, 
                             fcdpp_wrapper_async_cb_t cb,
                             void *ctx,
                             uint32_t buf_num,
                             uint32_t buf_len) {  /* int16_t's*/
 assert(0);
 return -1;
}

#endif


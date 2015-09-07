
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2014
//
// Copyright 2014, David Jacobowitz
*/

#include "dp_radiofcdpp.h"
#include "dp_conc_ringbuffer.h"

#include <string.h>


#ifdef __linux
#include <unistd.h>
void radio_sleep(int ims) {
 usleep(ims*1000);
}
#else
#ifdef __WIN32
#include <windows.h>
void radio_sleep(int ims) {
 Sleep(ims);
}
#endif
#endif

static const char *default_devname = "hw:2";

DP_SUB_CREATOR_IMPL(radiofcdpp)

DP_FN_PREAMBLE(radiofcdpp,init) {
 s->dev_is_open = 0;
 s->running_async = 0;
 s->dev = 0;
 s->ec = 0;
 s->report_overflow = 0;
 dp_conc_ringbuffer_init(&s->rb);
 b->complex_ok = 0;
 b->sub_work    = &dp_radiofcdpp_work;
 b->sub_deinit  = &dp_radiofcdpp_deinit;
 b->sub_prerun  = &dp_radiofcdpp_prerun;
 b->sub_postrun = &dp_radiofcdpp_postrun;
 s->devname = malloc(strlen(default_devname)+1);
 strcpy(s->devname,default_devname);
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(radiofcdpp,deinit) {
 if (s->running_async) {
  fcdpp_wrapper_cancel_async(s->dev); 
 }
 if (s->dev_is_open) {
  dp_radiofcdpp_close_device(b);
 }
 dp_conc_ringbuffer_deinit(&s->rb);
}
DP_FN_POSTAMBLE

void
dp_radiofcdpp_set_dev(dp_base_t *b, fcdpp_wrapper_t *id) {
 dp_radiofcdpp_sub_t *s = b->sub;
 if (DP_SUBVALID(b,radiofcdpp)) {
  s->dev = id; 
  s->dev_is_open = 1;
 }
}

void
dp_radiofcdpp_set_buffers(dp_base_t *b, uint32_t blen, uint32_t bcount) {
 dp_radiofcdpp_sub_t *s = b->sub;
 if (DP_SUBVALID(b,radiofcdpp)) {
  fprintf(stderr,"-ci- fcdpp (%s) creating %d deep buffer of size %d\n",
		  b->name,bcount,blen);
 dp_conc_ringbuffer_set_buffers(&s->rb,bcount,blen*2);
 b->runlength = blen;
 b->length_valid = 1;
 }
}

int
dp_radiofcdpp_close_device(dp_base_t *b) {
 int rv = 0;
 dp_radiofcdpp_sub_t *s = b->sub;
 if (DP_SUBVALID(b,radiofcdpp)) {
  if (s->dev_is_open) {
   rv = fcdpp_wrapper_close(s->dev);
   if (rv) {
    fprintf(stderr,"-ce- fcdpp could not close dev. return code %d\n",rv);
   }
   s->dev_is_open = 0;
  }
 }
 return rv;
}



/*
int
dp_radiofcdpp_list_devices(dp_base_t *b) {
 dp_radiofcdpp_sub_t *s = b->sub;
 ...
 return dev_count;
}
*/

int
dp_radiofcdpp_open_device(dp_base_t *b, char *n) {
 dp_radiofcdpp_sub_t *s = b->sub;
 int rv = 0;
 if (DP_SUBVALID(b,radiofcdpp)) {
  if (s->devname) {
   free(s->devname);
   s->devname = malloc(strlen(n)+1);
   strcpy(s->devname,n);
  }
  rv = fcdpp_wrapper_open(&s->dev,s->devname);
  if (rv) {
   fprintf(stderr,"-ce- fcdpp could not open dev. return code %d\n",rv);
  }
  s->dev_is_open = 1;
 } 
 return rv; 
}


DP_FN_PREAMBLE(radiofcdpp,stop_async) {
 if (s->dev_is_open && s->running_async) {
  fcdpp_wrapper_cancel_async(s->dev);
  s->running_async = 0;
 }
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(radiofcdpp,postrun) {
 dp_radiofcdpp_stop_async(b);
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(radiofcdpp,prerun) {
 s->report_overflow = 0;
}
DP_FN_POSTAMBLE


/* These getters are a kluge to let the non-objecty pthreads routines
// have access to data in this object. */
dp_conc_ringbuffer_t *
dp_radiofcdpp_get_rb(dp_base_t *b) {
 dp_radiofcdpp_sub_t *s = b->sub;
 if (DP_SUBVALID(b,radiofcdpp)) {
  return &s->rb;
 }
 return 0;
}

fcdpp_wrapper_t *
dp_radiofcdpp_get_dev(dp_base_t *b) {
 dp_radiofcdpp_sub_t *s = b->sub;
 if (DP_SUBVALID(b,radiofcdpp)) {
  return s->dev;
 }
 return 0;
}

dp_bool_t *
dp_radiofcdpp_get_running_async(dp_base_t *b) {
 dp_radiofcdpp_sub_t *s = b->sub;
 if (DP_SUBVALID(b,radiofcdpp)) {
  return &s->running_async;
 }
 return 0;
}

DP_SUB_GENERIC_GETTER_IMPL(radiofcdpp,report_overflow,dp_bool_t)
DP_SUB_GENERIC_GETTER_IMPL(radiofcdpp,curr_freq,uint32_t)

void 
dp_radiofcdpp_rb_callback(int16_t *ibuf, uint32_t len, void *f) {
 dp_base_t *b = (dp_base_t *)f;
 dp_radiofcdpp_sub_t *s = b->sub;
 dp_conc_ringbuffer_t *prb = &s->rb;
 dp_bool_t overflow = dp_conc_ringbuffer_store_buffer16b(prb, ibuf, len);
 if (overflow && dp_radiofcdpp_get_report_overflow(b)) { 
   fprintf(stderr, "-cw- (fcdpp) overflow (unread samples overwritten)\n");
 } 
}


int
dp_radiofcdpp_set_frequency(dp_base_t *b, uint32_t a) {
 dp_radiofcdpp_sub_t *s = b->sub;
 if (DP_SUBVALID(b,radiofcdpp)) {
  fcdpp_wrapper_set_frequency(s->dev,a);
 }
 return a;
}

DP_FN_PREAMBLE(radiofcdpp,work) {
 dp_vec_t *readbuffer = dp_conc_ringbuffer_get_curr_readbuffer(&s->rb,1);
 uint32_t i = 0;
 for (i=0;i<b->runlength;i++) {
  b->out_v->v[2*i]   = readbuffer->v[2*i];
  b->out_v->v[2*i+1] = readbuffer->v[2*i+1];
 }
}
DP_FN_POSTAMBLE

pthread_t *
dp_radiofcdpp_start_thread(dp_base_t *b) {
 pthread_t *rthread; 
 int err = 0;
 rthread = malloc(sizeof(pthread_t));
 if (!rthread) {
  fprintf(stderr,"-ce- (%s) could not create thread for callback\n",b->name);
  return 0;
 };
 err = pthread_create(rthread,NULL,dp_fcdpp_wrapper_thread_fn,b);
 if (err) {
  fprintf(stderr,"-ce- (%s) callback thread failed to start\n",b->name);
  return 0;
 };
 return rthread;
}


void *dp_fcdpp_wrapper_thread_fn(void *f) {
 dp_base_t *b = f;
 dp_radiofcdpp_sub_t *s = b->sub;
 dp_conc_ringbuffer_t *prb = &s->rb;
 int rv = 0;

 fprintf(stderr,"-ci- starting fcdpp_thread_fn (initiates callbacks)\n");

 /* the ringbuffer's buffers are allocated in the dsp thread, not this 
 // one, so we have to wait until that thread has gotten around to 
 // allocating those buffers, otherwise the calls to the ringbuffer
 // made by the callback specified below will generate exceptions.
 */
 while (!dp_conc_ringbuffer_buffers_set(prb)) {
  radio_sleep(100);
 }

 rv = fcdpp_wrapper_reset_buffer(s->dev);
 if (rv) {
  fprintf(stderr,"-ce- (%s) could not reset fcdpp buffer (%d)\n",b->name,rv);
 } else {
  s->running_async = 1;
  rv = fcdpp_wrapper_read_async(s->dev, &dp_radiofcdpp_rb_callback, f, 
		  dp_conc_ringbuffer_get_buff_count(prb),
		  dp_conc_ringbuffer_get_buff_len(prb));
  if (rv) {
   fprintf(stderr,"-ce- (%s) could not start read_async callback routine\n",b->name);
  } else {
   fprintf(stderr,"-ci- (%s) async buffer filling stopped\n",b->name);
  }
 }
 return 0;
}


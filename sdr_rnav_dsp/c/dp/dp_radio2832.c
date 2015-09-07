
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#include <rtl-sdr.h>
#include "dp_radio2832.h"
#include "dp_conc_ringbuffer.h"

#include <string.h>


#ifdef __linux
#include <unistd.h>
void rtl_sleep(int ims) {
 usleep(ims*1000);
}
#else
#ifdef __WIN32
#include <windows.h>
void rtl_sleep(int ims) {
 Sleep(ims);
}
#endif
#endif

/* Implementation of wrapper class for dealing with the RTL2832
// radio dongle, as provided by the rtl-sdr library.
// http://sdr.osmocom.org/trac/wiki/rtl-sdr
*/

DP_SUB_CREATOR_IMPL(radio2832)

DP_FN_PREAMBLE(radio2832,init) {
 s->dev_is_open = 0;
 s->running_async = 0;
 s->dev = 0;
 s->ec = 0;
 s->report_overflow = 0;
 s->gainslist = 0;
 s->gainscount = 0;
 dp_conc_ringbuffer_init(&s->rb);
 b->complex_ok = 0;
 b->sub_work    = &dp_radio2832_work;
 b->sub_deinit  = &dp_radio2832_deinit;
 b->sub_prerun  = &dp_radio2832_prerun;
 b->sub_postrun = &dp_radio2832_postrun;
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(radio2832,deinit) {
 if (s->running_async) {
  rtlsdr_cancel_async(s->dev); 
 }
 if (s->dev_is_open) {
  dp_radio2832_close_device(b);
 }
 dp_conc_ringbuffer_deinit(&s->rb);
}
DP_FN_POSTAMBLE

void
dp_radio2832_set_dev(dp_base_t *b, rtlsdr_dev_t *id) {
 dp_radio2832_sub_t *s = b->sub;
 if (DP_SUBVALID(b,radio2832)) {
  s->dev = id; 
  s->dev_is_open = 1;
 }
}

/* note on buffer length, blen. It is the length of the buffer in 
// time samples. Each of those has an I and Q component, so the 
// length of the actual buffer with be 2x as long.
*/
void
dp_radio2832_set_buffers(dp_base_t *b, uint32_t blen, uint32_t bcount) {
 dp_radio2832_sub_t *s = b->sub;
 if (DP_SUBVALID(b,radio2832)) {
  fprintf(stderr,"-ci- rtl_help (%s) creating %d deep buffer of size %d\n",
		  b->name,bcount,blen);
 dp_conc_ringbuffer_set_buffers(&s->rb,bcount,blen*2);
 b->runlength = blen;
 b->length_valid = 1;
 }
}

int
dp_radio2832_close_device(dp_base_t *b) {
 int rv = 0;
 dp_radio2832_sub_t *s = b->sub;
 if (DP_SUBVALID(b,radio2832)) {
  if (s->dev_is_open) {
   rv = rtlsdr_close(s->dev);
   s->dev_is_open = 0;
  }
 }
 return rv;
}



int
dp_radio2832_list_devices(dp_base_t *b) {
 dp_radio2832_sub_t *s = b->sub;
 int dev_count = rtlsdr_get_device_count();
 int i;
 if (dev_count >= 0) {
  for (i=0;i<dev_count;i++) {
   char tbuf[50];
   memset(tbuf,0,50);
   strncpy(tbuf,rtlsdr_get_device_name(i),49);
   fprintf(stderr,"-ci- (%s) device %d : %s\n",b->name,i,tbuf);
  }
 } else {
  fprintf(stderr,"-ce- rtlsdr: no devices?\n");
  s->ec |= 0x2;
  return 0;
 }
 return dev_count;
}

int
dp_radio2832_open_device(dp_base_t *b, uint32_t index) {
 dp_radio2832_sub_t *s = b->sub;
 int rv = 0;
 if (DP_SUBVALID(b,radio2832)) {
  rv = rtlsdr_open(&s->dev,index); 
  if (rv) {
   fprintf(stderr,"-ce- rtlsdr could not open dev. return code %d\n",rv);
  }
  s->dev_is_open = 1;
 } 
 return rv; 
}


DP_FN_PREAMBLE(radio2832,stop_async) {
 if (s->dev_is_open && s->running_async) {
  rtlsdr_cancel_async(s->dev);
  s->running_async = 0;
 }
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(radio2832,postrun) {
 dp_radio2832_stop_async(b);
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(radio2832,prerun) {
 s->report_overflow = 0;
}
DP_FN_POSTAMBLE


/* These getters are a kluge to let the non-objecty pthreads routines
// have access to data in this object. */
dp_conc_ringbuffer_t *
dp_radio2832_get_rb(dp_base_t *b) {
 dp_radio2832_sub_t *s = b->sub;
 if (DP_SUBVALID(b,radio2832)) {
  return &s->rb;
 }
 return 0;
}

rtlsdr_dev_t *
dp_radio2832_get_dev(dp_base_t *b) {
 dp_radio2832_sub_t *s = b->sub;
 if (DP_SUBVALID(b,radio2832)) {
  return s->dev;
 }
 return 0;
}

dp_bool_t *
dp_radio2832_get_running_async(dp_base_t *b) {
 dp_radio2832_sub_t *s = b->sub;
 if (DP_SUBVALID(b,radio2832)) {
  return &s->running_async;
 }
 return 0;
}

DP_SUB_GENERIC_GETTER_IMPL(radio2832,report_overflow,dp_bool_t)
DP_SUB_GENERIC_GETTER_IMPL(radio2832,curr_freq,uint32_t)

void 
dp_radio2832_rb_callback(unsigned char *ibuf, uint32_t len, void *f) {
 dp_base_t *b = (dp_base_t *)f;
 dp_radio2832_sub_t *s = b->sub;
 dp_conc_ringbuffer_t *prb = &s->rb;
 dp_bool_t overflow = dp_conc_ringbuffer_store_buffer8b(prb, ibuf, len);
 if (overflow && dp_radio2832_get_report_overflow(b)) { 
   fprintf(stderr, "-cw- (rtlsdr) overflow (unread samples overwritten)\n");
 } 
}


/* this function is for the perl access. It allows you to set through
 * 'a', but obviously, not get any return / read values
 */
int
dp_radio2832_dev_cmd_st_only(dp_base_t *b, rtl2832_cmd_t c, uint32_t a) {
 return dp_radio2832_dev_cmd(b,c,&a);
}

int dp_radio2832_get_tuner_gains(dp_base_t *b) {
 int ok = 0;
 int *gains = 0;
 int i = 0;
 dp_radio2832_sub_t *s = b->sub;
 if (DP_SUBVALID(b,radio2832)) {
  ok = rtlsdr_get_tuner_gains(s->dev,gains);
  if (ok > 0) {
   gains = malloc(sizeof(int) * ok);
   s->gainslist = gains;
   s->gainscount = ok;
   ok = rtlsdr_get_tuner_gains(s->dev,gains);
   if (ok >= 0) {
    for (i=0;i<s->gainscount;i++) {
     printf("gain idx[%d] = %d\n",i,s->gainslist[i]);
    }
   }
  }
 }
 return ok;
}


int
dp_radio2832_dev_cmd(dp_base_t *b, rtl2832_cmd_t c, uint32_t *a) {
 dp_radio2832_sub_t *s = b->sub;
 int rv = 0;

 if (DP_SUBVALID(b,radio2832)) {
  switch (c) {
   case ST_FREQ :
    /* fprintf(stderr,"-ci- (%s) changing freq to %d\n",b->name,*a); */
    rv = rtlsdr_set_center_freq(s->dev,*a);
    *a = rtlsdr_get_center_freq(s->dev);
    fprintf(stderr,"new freq %d\n",*a);
    s->curr_freq = *a;
    break;
   case GT_FREQ :
    *a = rtlsdr_get_center_freq(s->dev);
    rv = 0;
    break;
   case GT_SR :
    *a = rtlsdr_get_sample_rate(s->dev);
    rv = 0;
    break;
   case ST_SR :
    fprintf(stderr,"-ci- (%s) changing sample rate to %d\n",b->name,*a);
    rv = rtlsdr_set_sample_rate(s->dev,*a);
    *a = rtlsdr_get_sample_rate(s->dev);
    break;
   case ST_AGC :
    fprintf(stderr,"-ci- (%s) rtlsdr_set_agc_mode(%d)\n",b->name,*a);
    rv = rtlsdr_set_agc_mode(s->dev,*a);
    break;
   case ST_TUNER_GAIN_MODE:
    fprintf(stderr,"-ci- (%s) rtlsdr_set_tuner_gain_mode(%d)\n",b->name,*a);
    rv = rtlsdr_set_tuner_gain_mode(s->dev,*a);
    break;
   case ST_TUNER_GAIN:
    fprintf(stderr,"-ci- (%s) rtlsdr_set_tuner_gain(%d)\n",b->name,*a);
    rv = rtlsdr_set_tuner_gain(s->dev,*a);
    break;
   case ST_TEST:
    fprintf(stderr,"-ci- (%s) rtlsdr_set_testmode(%d)\n",b->name,*a);
    rv = rtlsdr_set_testmode(s->dev,*a);
    break;
   default:
    break;
  } 
  if (rv) {
   fprintf(stderr,"-ce- rtlsdr: dev_cmd failed cmd %d, %d\n",(int)c,rv);
  }
 }
 return rv;
}

DP_FN_PREAMBLE(radio2832,work) {
 dp_vec_t *readbuffer = dp_conc_ringbuffer_get_curr_readbuffer(&s->rb,1);
 uint32_t i = 0;
 for (i=0;i<b->runlength;i++) {
  b->out_v->v[2*i]   = readbuffer->v[2*i];
  b->out_v->v[2*i+1] = readbuffer->v[2*i+1];
 }
}
DP_FN_POSTAMBLE

pthread_t *
dp_radio2832_start_thread(dp_base_t *b) {
 pthread_t *rthread; 
 int err = 0;
 rthread = malloc(sizeof(pthread_t));
 if (!rthread) {
  fprintf(stderr,"-ce- (%s) could not create thread for callback\n",b->name);
  return 0;
 };
 err = pthread_create(rthread,NULL,rtl2832_thread_fn,b);
 if (err) {
  fprintf(stderr,"-ce- (%s) callback thread failed to start\n",b->name);
  return 0;
 };
 return rthread;
}


void *rtl2832_thread_fn(void *f) {
 dp_base_t *b = f;
 dp_radio2832_sub_t *s = b->sub;
 dp_conc_ringbuffer_t *prb = &s->rb;
 int rv = 0;

 fprintf(stderr,"-ci- starting rtl2832_thread_fn (initiates callbacks)\n");

 /* the ringbuffer's buffers are allocated in the dsp thread, not this 
 // one, so we have to wait until that thread has gotten around to 
 // allocating those buffers, otherwise the calls to the ringbuffer
 // made by the callback specified below will generate exceptions.
 */
 while (!dp_conc_ringbuffer_buffers_set(prb)) {
  rtl_sleep(100);
 }

 rv = rtlsdr_reset_buffer(s->dev);
 if (rv) {
  fprintf(stderr,"-ce- (%s) could not reset rtl buffer (%d)\n",b->name,rv);
 } else {
  s->running_async = 1;
  rv = rtlsdr_read_async(s->dev, &dp_radio2832_rb_callback, f, 
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


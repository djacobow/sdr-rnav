
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2015
//
// Copyright 2015, David Jacobowitz
*/

static const int MAX_SR_COUNT = 3;

#include <libairspy/airspy.h>
#include "dp_airspy.h"
#include "dp_conc_ringbuffer.h"

#include <string.h>


#ifdef __linux
#include <unistd.h>
void airspy_sleep(int ims) {
 usleep(ims*1000);
}
#else
#ifdef __WIN32
#include <windows.h>
void airspy_sleep(int ims) {
 Sleep(ims);
}
#endif
#endif


DP_SUB_CREATOR_IMPL(airspy)

DP_FN_PREAMBLE(airspy,init) {
 s->dev_is_open = 0;
 s->running_async = 0;
 s->dev = 0;
 s->ec = 0;
 s->report_overflow = 0;
 s->srlist = 0;
 s->srcount= 0;
 dp_conc_ringbuffer_init(&s->rb);
 b->complex_ok = 0;
 b->sub_work    = &dp_airspy_work;
 b->sub_deinit  = &dp_airspy_deinit;
 b->sub_prerun  = &dp_airspy_prerun;
 b->sub_postrun = &dp_airspy_postrun;
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(airspy,deinit) {
 if (s->running_async) {
  airspy_stop_rx(s->dev);
 }
 if (s->dev_is_open) {
  dp_airspy_close_device(b);
 }
 dp_conc_ringbuffer_deinit(&s->rb);
}
DP_FN_POSTAMBLE

void
dp_airspy_set_dev(dp_base_t *b, struct airspy_device *id) {
 dp_airspy_sub_t *s = b->sub;
 if (DP_SUBVALID(b,airspy)) {
  s->dev = id; 
  s->dev_is_open = 1;
 }
}

/* note on buffer length, blen. It is the length of the buffer in 
// time samples. Each of those has an I and Q component, so the 
// length of the actual buffer with be 2x as long.
*/
void
dp_airspy_set_buffers(dp_base_t *b, uint32_t blen, uint32_t bcount) {
 dp_airspy_sub_t *s = b->sub;
 if (DP_SUBVALID(b,airspy)) {
  fprintf(stderr,"-ci- rtl_help (%s) creating %d deep buffer of size %d\n",
		  b->name,bcount,blen);
 dp_conc_ringbuffer_set_buffers(&s->rb,bcount,blen*2);
 b->runlength = blen;
 b->length_valid = 1;
 }
}

int
dp_airspy_close_device(dp_base_t *b) {
 int rv = 0;
 dp_airspy_sub_t *s = b->sub;
 if (DP_SUBVALID(b,airspy)) {
  if (s->dev_is_open) {
   rv = airspy_close(s->dev);
   s->dev_is_open = 0;
  }
 }
 return rv;
}



int
dp_airspy_open_device(dp_base_t *b, uint32_t sn) {
 dp_airspy_sub_t *s = b->sub;
 int rv = 0;
 if (DP_SUBVALID(b,airspy)) {
  if (sn) {
   rv = airspy_open_sn(&s->dev,sn); 
  } else {
   rv = airspy_open(&s->dev); 
  }
  if (rv) {
   fprintf(stderr,"-ce- airspy could not open dev. return code %d\n",rv);
  }
  airspy_set_sample_type(s->dev, AIRSPY_SAMPLE_INT16_IQ);
  s->dev_is_open = 1;
 } 
 return rv; 
}


DP_FN_PREAMBLE(airspy,stop_async) {
 if (s->dev_is_open && s->running_async) {
  airspy_stop_rx(s->dev);
  s->running_async = 0;
 }
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(airspy,postrun) {
 dp_airspy_stop_async(b);
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(airspy,prerun) {
 s->report_overflow = 0;
}
DP_FN_POSTAMBLE


/* These getters are a kluge to let the non-objecty pthreads routines
// have access to data in this object. */
dp_conc_ringbuffer_t *
dp_airspy_get_rb(dp_base_t *b) {
 dp_airspy_sub_t *s = b->sub;
 if (DP_SUBVALID(b,airspy)) {
  return &s->rb;
 }
 return 0;
}

struct airspy_device *
dp_airspy_get_dev(dp_base_t *b) {
 dp_airspy_sub_t *s = b->sub;
 if (DP_SUBVALID(b,airspy)) {
  return s->dev;
 }
 return 0;
}

dp_bool_t *
dp_airspy_get_running_async(dp_base_t *b) {
 dp_airspy_sub_t *s = b->sub;
 if (DP_SUBVALID(b,airspy)) {
  return &s->running_async;
 }
 return 0;
}

DP_SUB_GENERIC_GETTER_IMPL(airspy,report_overflow,dp_bool_t)
DP_SUB_GENERIC_GETTER_IMPL(airspy,curr_freq,uint32_t)

int 
*dp_airspy_rb_callback(airspy_transfer *transfer) {
 dp_base_t *b = (dp_base_t *)transfer->ctx;
 dp_airspy_sub_t *s = b->sub;
 dp_conc_ringbuffer_t *prb = &s->rb;

 if (transfer->sample_type == AIRSPY_SAMPLE_INT16_IQ) {
  dp_bool_t overflow = dp_conc_ringbuffer_store_buffer16b(prb,transfer->samples,transfer->sample_count);
  if (overflow && dp_airspy_get_report_overflow(b)) {
   fprintf(stderr, "-cw- (airspy) overflow (unread samples overwritten)\n");
  }
 } else {
   fprintf(stderr, "-cw- (airspy) not 16b IQ mode\n");
 }
}


/* this function is for the perl access. It allows you to set through
 * 'a', but obviously, not get any return / read values
 */
int
dp_airspy_dev_cmd_st_only(dp_base_t *b, airspy_cmd_t c, uint32_t a) {
 return dp_airspy_dev_cmd(b,c,&a);
}


int dp_airspy_get_samplerates(dp_base_t *b) {
 int ok = 0;
 int i = 0;
 dp_airspy_sub_t *s = b->sub;
 if (DP_SUBVALID(b,airspy)) {
  s->srlist = malloc(sizeof(uint32_t) * MAX_SR_COUNT);
  ok = airspy_get_samplerates(s->dev,s->srlist,MAX_SR_COUNT);
  if (ok == 0) {
   for (i=0;i<MAX_SR_COUNT;i++) {
    printf("sample rate idx[%d] = %d\n",i,s->srlist[i]);
   }
  }
 }
 return ok;
}


int
dp_airspy_dev_cmd(dp_base_t *b, airspy_cmd_t c, uint32_t *a) {
 dp_airspy_sub_t *s = b->sub;
 int rv = 0;

 if (DP_SUBVALID(b,airspy)) {
  switch (c) {
   case AS_ST_FREQ :
    /* fprintf(stderr,"-ci- (%s) changing freq to %d\n",b->name,*a); */
    rv = airspy_set_freq(s->dev,*a);
    fprintf(stderr,"new freq %d\n",*a);
    s->curr_freq = *a;
    break;
   case AS_ST_SR :
    fprintf(stderr,"-ci- (%s) changing sample rate to %d\n",b->name,*a);
    rv = airspy_set_samplerate(s->dev,*a);
    break;
   case AS_ST_VGA_GAIN:
    fprintf(stderr,"-ci- (%s) vga gain (%d)\n",b->name,*a);
    rv = airspy_set_vga_gain(s->dev,*a);
    break;
   case AS_ST_MIX_GAIN:
    fprintf(stderr,"-ci- (%s) mixer gain (%d)\n",b->name,*a);
    rv = airspy_set_mixer_gain(s->dev,*a);
    break;
   case AS_ST_LNA_GAIN:
    fprintf(stderr,"-ci- (%s) lna gain (%d)\n",b->name,*a);
    rv = airspy_set_lna_gain(s->dev,*a);
    break;
   case AS_ST_MIX_AGC:
    fprintf(stderr,"-ci- (%s) mixer agc (%d)\n",b->name,*a);
    rv = airspy_set_mixer_agc(s->dev,*a);
    break;
   case AS_ST_LNA_AGC:
    fprintf(stderr,"-ci- (%s) lna agc (%d)\n",b->name,*a);
    rv = airspy_set_lna_agc(s->dev,*a);
    break;
   default:
    break;
  } 
  if (rv) {
   fprintf(stderr,"-ce- airspy : dev_cmd failed cmd %d, %d\n",(int)c,rv);
  }
 }
 return rv;
}

DP_FN_PREAMBLE(airspy,work) {
 dp_vec_t *readbuffer = dp_conc_ringbuffer_get_curr_readbuffer(&s->rb,1);
 uint32_t i = 0;
 for (i=0;i<b->runlength;i++) {
  b->out_v->v[2*i]   = readbuffer->v[2*i];
  b->out_v->v[2*i+1] = readbuffer->v[2*i+1];
 }
}
DP_FN_POSTAMBLE

pthread_t *
dp_airspy_start_thread(dp_base_t *b) {
 pthread_t *rthread; 
 int err = 0;
 rthread = malloc(sizeof(pthread_t));
 if (!rthread) {
  fprintf(stderr,"-ce- (%s) could not create thread for callback\n",b->name);
  return 0;
 };
 err = pthread_create(rthread,NULL,airspy_thread_fn,b);
 if (err) {
  fprintf(stderr,"-ce- (%s) callback thread failed to start\n",b->name);
  return 0;
 };
 return rthread;
}


void *airspy_thread_fn(void *f) {
 dp_base_t *b = f;
 dp_airspy_sub_t *s = b->sub;
 dp_conc_ringbuffer_t *prb = &s->rb;
 int rv = 0;

 fprintf(stderr,"-ci- starting airspy_thread_fn (initiates callbacks)\n");

 /* the ringbuffer's buffers are allocated in the dsp thread, not this 
 // one, so we have to wait until that thread has gotten around to 
 // allocating those buffers, otherwise the calls to the ringbuffer
 // made by the callback specified below will generate exceptions.
 */
 while (!dp_conc_ringbuffer_buffers_set(prb)) {
  rtl_sleep(100);
 }

 rv = 0;
 if (rv) {
  fprintf(stderr,"-ce- (%s) could not reset rtl buffer (%d)\n",b->name,rv);
 } else {
  s->running_async = 1;
  rv = airspy_start_rx(s->dev, dp_airspy_rb_callback, f);

//  rv = rtlsdr_read_async(s->dev, &dp_airspy_rb_callback, f, 
//		  dp_conc_ringbuffer_get_buff_count(prb),
//		  dp_conc_ringbuffer_get_buff_len(prb));
  if (rv) {
   fprintf(stderr,"-ce- (%s) could not start read_async callback routine\n",b->name);
  } else {
   fprintf(stderr,"-ci- (%s) async buffer filling stopped\n",b->name);
  }
 }
 return 0;
}


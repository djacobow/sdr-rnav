
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz
*/

#include <stdio.h>
#include "ctrl_thread.h"
#include "receiver_stat.h"
#include <inttypes.h>
#include "main.h"
#include <assert.h>
#include "dsp_chain.h"
#include "my_qs.h"
#include "my_console.h"
#include "dp_radio2832.h"
#include "dp_findpeaks.h"


/* This is to turn off uninitialized warnings on the "lrstat" variables,
// which are set in parameter functions rather than on the left side of
// equal signs
*/
#pragma GCC diagnostic ignored "-Wuninitialized"

/*

This function (ctrl_thread_fn) runs asynchronously from the radio+dsp
and essentially treats the radio as a separate, asynchronous "thing:" it
can change the radio's frequency and it can examine what's coming out of
the radio, that's it. The radio's output consists of two datastructures
passed to the outside world. 

One is receiver_stat_t a dumb struct that contains output results of
signal decoding.

The other is a std::string that contains the results of morse
decoding. These are passed out through two concurrency-friendly queues.
For the signal results queue, we enforce a maximum queue length; the
results of older buffers are just tossed if they haven't been looked
at. For the morse id queue, we are not enforcing a maximum length. It
is up to the consumer to consume often enough that it doesn't grow
ridiculously large.

There is actually one more variable, _dsp_chain_all_done, which the
receiver will set to true when it is shutting down. Other threads
can check it to see if they should shut down. It does not have mutex
protection. These three varaibles are at file scope in dsp_chain.cpp
and marked extern in dsp_chain.h

*/


/* For tuning in real-time using the arrow keys, these are the 
   sizes of the increments */
const int freqIncrLarge         = 10000;
const int freqIncrSmall         = 1000;
const unsigned int loIncr       = 1000;
const unsigned int loMin        = 2000;


void
fft_peaks_to_json(char *dst, peak_pts_t *pts) {
 char ptln[SHORT_STRING_LENGTH];
 uint32_t i;
 snprintf(dst,LONG_STRING_LENGTH,
"\
{ \"ok\": 1,  \
  \"length\": %d, \
  \"average\": %d, \
  \"iteration\": %d, \
  \"points\": [ \
",pts->length,pts->average,pts->iteration);
 for (i=0;i<pts->actpts;i++) {
  snprintf(ptln, SHORT_STRING_LENGTH,
		  "{ \"idx\": %d, \"db\": %f, \"abs\": %d }",
		  pts->points[i]->bin,pts->points[i]->db,pts->points[i]->abs);
  strlcat(dst,ptln,LONG_STRING_LENGTH);
  if (i < pts->actpts-1) {
   strlcat(ptln,",",SHORT_STRING_LENGTH);
  }
 }
 strlcat(dst, "] }", LONG_STRING_LENGTH);
} 


void
stats_to_json(char *dst, receiver_stat_t *prs, char *idstr) {
 char more[SHORT_STRING_LENGTH];
 dst[0] = 0;
 /* something is going wrong here, but only on the RPi and windows
  * and linux snprintf works fine, but on RPi it segs out. I think
  * there is some kind of race and you see it in the slow RPi. -- dgj */

 snprintf(dst, LONG_STRING_LENGTH,
" { \
 \"have_carrier\": %c, \
 \"snr\": %f, \
 \"snr_lpf\": %f \
 \"id_instr\" : \"%s\", \
 \"freq\": %d, \
 \"use_mixer\": %d, \
 \"run_fft\": %d, \
 \"mixer_lo\": %d, \
 \"buffer_ct\": %d, \
", prs->have_carrier ? '1' : '0',
   prs->strength_ratio,
   prs->strength_ratio_lpf,
   idstr, 
   prs->tune_freq,
   prs->use_mixer,
   prs->run_fft,
   prs->mixer_lo_freq,
   prs->buffer_count);

 if (prs->have_carrier) {
  float period        = prs->nf_sr / 30.0; /* period in samples */
  float cyc_fract     = prs->phase_diff * 30.0;
  float cyc_fract_lpf = prs->phase_diff_lpf * 30.0;
  float angle     = (360.0 * cyc_fract     + _main_radial_calibrate);
  float angle_lpf = (360.0 * cyc_fract_lpf + _main_radial_calibrate);
  if (angle < 0)     { angle += 360.0; };
  if (angle_lpf < 0) { angle_lpf += 360.0; };
 
  snprintf(more,SHORT_STRING_LENGTH,
" \
 \"ref30_period\": %f, \
 \"var30_period\": %f, \
 \"radial\": %f, \
 \"radial_lpf\": %f, \
",prs->ref30_period,
  prs->var30_period,
  angle, angle_lpf);
  strlcat(dst,more,LONG_STRING_LENGTH);
 }
 strlcat(dst," \"ok\": 1 }",LONG_STRING_LENGTH);
}


char ostr[LONG_STRING_LENGTH+1];

void *ctrl_thread_fn(void *f) {
 
 uint32_t last_buffer    = 0;
 receiver_stat_t lrstat;
 peak_pts_t      pts;
 char id_instr[SHORT_STRING_LENGTH];

 fprintf(stderr, "-info- (ctrl) starting ctrl routine\n");
 _main_ctrl_has_begun |= 1;

 while (!_dsp_chain_all_done) {
  dp_bool_t have_status = dp_conc_q_rstat_try_pop_all(&_dsp_chain_rstat_queue,
		                                      &lrstat);
  dp_bool_t have_fft    = dp_conc_q_peaks_try_pop_all(&_dsp_chain_peaks_queue,
		                                      &pts);
  if (have_fft) {
   /* char ostr[LONG_STRING_LENGTH+1]; */
   fft_peaks_to_json(ostr,&pts);
   fputs(ostr,stdout);
   fputs("\n",stdout);
  }

  if (have_status) {
   char a_bit;

   id_instr[0] = 0;

   while (dp_conc_q_char_try_pop(&_dsp_chain_id_text_queue, &a_bit)) {
    int l = strlen(id_instr);
    id_instr[l] = a_bit;
    id_instr[l+1] = 0;
   }

   /* shorten string by shifting if necessary */

   if (lrstat.buffer_count != last_buffer) {
    /* char ostr[LONG_STRING_LENGTH+1]; */
    stats_to_json(ostr,&lrstat,id_instr);
    fputs(ostr,stdout);
    fputs("\n",stdout);
   }
  }

#ifdef USE_RADIO
  if (!have_status) {
   /*my_sleep(lrstat.block_time_ms/4); */
   my_sleep(100);
  } 
#endif

  last_buffer = lrstat.buffer_count;
 }
 return 0;
}



/*	$OpenBSD: strlcpy.c,v 1.11 2006/05/05 15:27:38 millert Exp $	*/

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}

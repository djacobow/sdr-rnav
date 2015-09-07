/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Fall 2013
//
// Copyright 2013, David Jacobowitz
*/

/* Like so many modules here, this one is another quick-and-dirty
 * not-quite-correct, but good enough for now kind of thing.
 *
 * This one uses the power of the last several frames of data
 * (as specified in 'time_constant' -- the time constant being 
 * the number of buffers not a time of number of samples.)
 *
 * Overall amplitude is scaled by a factor to scale the energy
 * to the desired value. This has to be tuned by hand. I guess
 * the right way would be to scale automatically to just shy of 
 * overflow.
 */

#include "dp_agc.h"
#include "dp_c2r.h"
#include <math.h>
#include <stdio.h>

DP_SUB_CREATOR_IMPL(agc)

DP_FN_PREAMBLE(agc,init) {
 s->time_constant = 0.8;
 s->avg_mag_tconst_frames = 0;
 s->desired_mag = 8192;
 b->sub_work   = &dp_agc_work;
}
DP_FN_POSTAMBLE

DP_SUB_GENERIC_SETTER_IMPL(agc,time_constant,float)

DP_SUB_GENERIC_SETTER_IMPL(agc,desired_mag,float)

DP_FN_PREAMBLE(agc,work) {
 uint32_t sum = 0;
 float avg_mag_this_frame;
 float mul_factor_f;
 uint32_t mul_factor_i;
 uint32_t i;

 /* First, determine the power for this block of samples. */
 if (b->is_complex) {
  for (i=0;i<b->runlength;i++) {
   uint32_t v = dp_comp16tomag16(b->in_v->v[2*i], b->in_v->v[2*i+1]);
   sum += (v * v) >> 16;
  }
 } else {
  for (i=0;i<b->runlength;i++) {
   sum += ((uint32_t)b->in_v->v[i] * (uint32_t)b->in_v->v[i]) >> 16;
  }
 }

 /* printf("sum %d rl %d\n",sum, b->runlength); */
 /* avg_mag_this_frame = (float)dp_SquareRoot(sum/b->runlength); */

 /* This is the RMS power */
 avg_mag_this_frame = sqrt((float)sum/(float)b->runlength);

 /* This is the RMS power with a simple one pole IIR LPF over the 
  * last few frames. */
 s->avg_mag_tconst_frames = (float)s->avg_mag_tconst_frames * s->time_constant +
	                    (float)avg_mag_this_frame * (1-s->time_constant);

 /* zero is not ok */
 if (s->avg_mag_tconst_frames < 1) {
  s->avg_mag_tconst_frames = 1;
 }

 /* in case the signal is too large and we want to be able to
  * shrink it, create a multiplier that is 8 bits left shifted.
  * We will right shift by eight bits so that 1x is 1x 
  */
 mul_factor_f = 256 * s->desired_mag / s->avg_mag_tconst_frames;
 if (mul_factor_f > 32767) { mul_factor_f = 32767; };

 /*
 printf("avg_mag_this_frame %f avg_mag_tconst %f mul_factor_f %f desired %f\n",avg_mag_this_frame,
		 s->avg_mag_tconst_frames,mul_factor_f,s->desired_mag);
 */

 /* enough floats, back to ints */
 mul_factor_i = floor(mul_factor_f);

 /* and scale */
 if (b->is_complex) {
  for (i=0;i<b->runlength*2;i++) {
   b->out_v->v[i] = ((int32_t)b->in_v->v[i] * mul_factor_i) >> 8;
  };
 } else {
  for (i=0;i<b->runlength;i++) {
   b->out_v->v[i] = ((int32_t)b->in_v->v[i] * mul_factor_i) >> 8;
  };
 }
}
DP_FN_POSTAMBLE


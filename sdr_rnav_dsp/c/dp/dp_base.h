
#ifndef __DP_BASE_H
#define __DP_BASE_H

#include "dp_types.h"
#include "dp_vector.h"
#include "dp_helpers.h"
#include <assert.h>

typedef enum dp_subt_t {
 dp_subt_null,
 dp_subt_wav_r,
 dp_subt_wav_w,
 dp_subt_fir,
 dp_subt_c2r,
 dp_subt_decimate,
 dp_subt_cmix,
 dp_subt_mul,
 dp_subt_sum,
 dp_subt_scale,
 dp_subt_ph_compare,
 dp_subt_radio2832,
 dp_subt_radiofcdpp,
 dp_subt_fft_fir,
 dp_subt_interleave,
 dp_subt_deinterleave,
 dp_subt_thresh,
 dp_subt_ditdah,
 dp_subt_linaudio,
 dp_subt_cfft,
 dp_subt_dcblock,
 dp_subt_envelope,
 dp_subt_ecompare,
 dp_subt_text,
 dp_subt_findpeaks,
 dp_subt_morse,
 dp_subt_quasiquad,
 dp_subt_makesig,
 dp_subt_iq_w,
 dp_subt_agc,
 dp_subt_airspy,
 __dp_subt_invalid
} dp_subt_t;


typedef struct dp_base_t dp_base_t;

typedef void (*dp_v_v_fn_t)(dp_base_t *);
typedef void (*dp_v_i_fn_t)(dp_base_t *, int32_t);
typedef void (*dp_v_ii_fn_t)(dp_base_t *, int32_t, int32_t);
typedef void (*dp_v_f_fn_t)(dp_base_t *, dp_float_t);
typedef void (*dp_v_ff_fn_t)(dp_base_t *, dp_float_t, dp_float_t);

struct dp_base_t {
 dp_bool_t base_valid;
 dp_bool_t sub_valid;
 const dp_vec_t *in_v;
 dp_vec_t *out_v;
 uint32_t runlength;
 char *name;
 uint32_t group;
 dp_bool_t length_valid;
 dp_bool_t complex_ok;
 dp_bool_t is_complex;
 void   *sub;
 dp_subt_t subt;
 dp_v_v_fn_t sub_work;
 dp_v_v_fn_t sub_prerun;
 dp_v_v_fn_t sub_postrun;
 dp_v_v_fn_t sub_deinit;
};

dp_base_t *dp_base_create();
void dp_base_create_from(dp_base_t *);
void dp_prerun(dp_base_t *b);
void dp_postrun(dp_base_t *b);
void dp_run(dp_base_t *b,uint32_t rmask);
void dp_set_in(dp_base_t *b, dp_vec_t *i);
void dp_set_inlt(dp_base_t *b, dp_vec_t *i, uint32_t il, dp_bool_t x);
void dp_set_inl(dp_base_t *b, dp_vec_t *i, uint32_t il);
void dp_set_out(dp_base_t *b, dp_vec_t *i);
void dp_set_group(dp_base_t *b, uint32_t group);
void dp_nullfn(dp_base_t *b);
void dp_base_deinit(dp_base_t *b);
void dp_set_runlen(dp_base_t *b, uint32_t l);
void dp_set_complex(dp_base_t *b, dp_bool_t i);
void dp_set_name(dp_base_t *b, const char *in);
char *dp_get_name(dp_base_t *b);
void dp_debug_valid(dp_base_t *b, dp_subt_t t);
void dp_destroy(dp_base_t *b);

#define DP_CREATE(b,t) \
 b = dp_base_create(); \
 dp_ ## t ## _create_from(b);

#define DP_CREATE_FROM(b,t) \
 dp_base_create_from(b); \
 dp_ ## t ## _create_from(b);

#endif

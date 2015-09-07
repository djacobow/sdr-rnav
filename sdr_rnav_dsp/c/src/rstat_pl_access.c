/*
Autogenerated by C:\svn\sdr-rnav\sdr_rnav_dsp\c\inc\make_receiver_stat.pl
on Thu Oct 10 16:31:19 2013
from receiver_stat.h

*/
#include "receiver_stat.h"
#include "rstat_pl_access.h"
void dp_pl_rstat_set_strength_ratio(receiver_stat_t *r,float  v) {
 r->strength_ratio = v;
}
float  dp_pl_rstat_get_strength_ratio(receiver_stat_t *r) {
 return r->strength_ratio;
}
void dp_pl_rstat_set_have_carrier(receiver_stat_t *r,dp_bool_t  v) {
 r->have_carrier = v;
}
dp_bool_t  dp_pl_rstat_get_have_carrier(receiver_stat_t *r) {
 return r->have_carrier;
}
void dp_pl_rstat_set_run_fft(receiver_stat_t *r,dp_bool_t  v) {
 r->run_fft = v;
}
dp_bool_t  dp_pl_rstat_get_run_fft(receiver_stat_t *r) {
 return r->run_fft;
}
void dp_pl_rstat_set_use_mixer(receiver_stat_t *r,dp_bool_t  v) {
 r->use_mixer = v;
}
dp_bool_t  dp_pl_rstat_get_use_mixer(receiver_stat_t *r) {
 return r->use_mixer;
}
void dp_pl_rstat_set_strength_ratio_lpf(receiver_stat_t *r,float  v) {
 r->strength_ratio_lpf = v;
}
float  dp_pl_rstat_get_strength_ratio_lpf(receiver_stat_t *r) {
 return r->strength_ratio_lpf;
}
void dp_pl_rstat_set_tune_freq(receiver_stat_t *r,uint32_t  v) {
 r->tune_freq = v;
}
uint32_t  dp_pl_rstat_get_tune_freq(receiver_stat_t *r) {
 return r->tune_freq;
}
void dp_pl_rstat_set_mixer_lo_freq(receiver_stat_t *r,int32_t  v) {
 r->mixer_lo_freq = v;
}
int32_t  dp_pl_rstat_get_mixer_lo_freq(receiver_stat_t *r) {
 return r->mixer_lo_freq;
}
void dp_pl_rstat_set_ref30_period(receiver_stat_t *r,float  v) {
 r->ref30_period = v;
}
float  dp_pl_rstat_get_ref30_period(receiver_stat_t *r) {
 return r->ref30_period;
}
void dp_pl_rstat_set_nf_sr(receiver_stat_t *r,uint32_t  v) {
 r->nf_sr = v;
}
uint32_t  dp_pl_rstat_get_nf_sr(receiver_stat_t *r) {
 return r->nf_sr;
}
void dp_pl_rstat_set_buffer_count(receiver_stat_t *r,uint32_t  v) {
 r->buffer_count = v;
}
uint32_t  dp_pl_rstat_get_buffer_count(receiver_stat_t *r) {
 return r->buffer_count;
}
void dp_pl_rstat_set_var30_period(receiver_stat_t *r,float  v) {
 r->var30_period = v;
}
float  dp_pl_rstat_get_var30_period(receiver_stat_t *r) {
 return r->var30_period;
}
void dp_pl_rstat_set_radio(receiver_stat_t *r,dp_base_t * v) {
 r->radio = v;
}
dp_base_t * dp_pl_rstat_get_radio(receiver_stat_t *r) {
 return r->radio;
}
void dp_pl_rstat_set_phase_diff_lpf(receiver_stat_t *r,float  v) {
 r->phase_diff_lpf = v;
}
float  dp_pl_rstat_get_phase_diff_lpf(receiver_stat_t *r) {
 return r->phase_diff_lpf;
}
void dp_pl_rstat_set_phase_diff(receiver_stat_t *r,float  v) {
 r->phase_diff = v;
}
float  dp_pl_rstat_get_phase_diff(receiver_stat_t *r) {
 return r->phase_diff;
}
void dp_pl_rstat_set_mixer(receiver_stat_t *r,dp_base_t * v) {
 r->mixer = v;
}
dp_base_t * dp_pl_rstat_get_mixer(receiver_stat_t *r) {
 return r->mixer;
}
void dp_pl_rstat_set_block_time_ms(receiver_stat_t *r,uint32_t  v) {
 r->block_time_ms = v;
}
uint32_t  dp_pl_rstat_get_block_time_ms(receiver_stat_t *r) {
 return r->block_time_ms;
}

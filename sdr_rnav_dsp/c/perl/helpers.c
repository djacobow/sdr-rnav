#include "dp_findpeaks.h"
#include "dp_radio2832.h"
#include "dp_pl_sharelist.h"

/* C wrapper to initiate callback to Perl-land. This will be
 * called after all the items in the baselist have been run()
 * but before the next buffer iteration. One perl scalar is 
 * passed in.
 */
void dsp_after_cb_caller(SV *a) {
 dSP;
 PUSHMARK(SP);
 /* XPUSHs(sv_2mortal(newSVpvf("Plus an extra line"))); */
 /* XPUSHs(sv_2mortal(newSVpv(n,0))); */
 XPUSHs(a);
 PUTBACK;
 call_pv("main::perl_dsp_after_cb", G_DISCARD);
}

/* wrapper around malloc that allow perl to alloc a
 * plain-c peak_pts_t
 */
DP_PL_ALLOC_TYPE_DECL(peak_pts_t);
DP_PL_ALLOC_TYPE_IMPL(peak_pts_t);

/* The only file-level global here, this is a pointer to 
 * a struct that will hold various variables for quick access
 * from perl and from C, from any thread. The variables in this
 * struct are not mutex protected, however. */
dp_pl_sharelist_t *sl;

/* routine to set the pointer above */
dp_pl_sharelist_t *set_sl(dp_pl_sharelist_t *isl) {
 sl = isl;
 return isl;
}

/* convert a vector to a perl array */
SV *dp_vec_to_perl(dp_vec_t *v) {
 int i   = 0;
 AV *array;
 int len = dp_vec_getlen(v);
 array = newAV();
 if (v->valid) {
  for (i=0;i<len;i++) {
   av_push(array,newSViv(v->v[i]));
  } 
 }
 return newRV_noinc((SV*)array);
}

/* get value at vector location into perl 
 * scalar
 */ 
SV *dp_vec_at(dp_vec_t *v, uint32_t idx) {
 SV *a = newSViv(0);;
 if (v->valid) {
  if (idx < v->len) {
   sv_setiv(a,v->v[idx]); 
  }
 }
 return a;
};

/* set value at vector location from perl scalar
 *
 */
void dp_vec_set_at_perl(dp_vec_t *v, uint32_t idx, SV *in) {
 if (v->valid && (idx < v->len)) {
  v->v[idx] = SvIV(in);
 } 
};


/* convert a perl array to a c array of int16s. Note 
 * that the c array is malloc'd here, so it that array
 * should be free'd at the end of use. In actuality in this
 * program, I'm not doing that yet. I use this routine 
 * primarily to convert the FIR filter coefficients to a C
 * array for use by the dp FIR objects. But they will not 
 * consume the pointer until they are prerun(), so the 
 * best time to free those pointers is not until done with
 * the dp object, so just prior to reaping them -- which 
 * this program does not even do since it was not designed 
 * with the idea of setting up, tearing down, and resetting
 * signal chains, etc -- just setting up once.
 */
int16_t *perlToCArray_dp_int_t(AV *pa) {
 int i;
 dp_int_t *ca;

 ca = malloc(sizeof(dp_int_t) * av_len(pa));
 if (ca) {
  for (i=0;i<av_len(pa);i++) {
   SV** pelem = av_fetch(pa,i,0);
   if (pelem != NULL) {
    ca[i] = SvNV(*pelem);
   }
  }
 }
 return ca;
}


/* C wrapper function that run the entire dsp show. Typically
 * run in its own thread, this routine takes a pointer to a 
 * dp_baselist_t which itself contains a list of dp object modules.
 *
 * It calls prerun() on all of them, then run()'s them until 
 * 'done', and finally calls postrun() on all of them. After 
 * each iteration of run() it also calls back to a perl subroutine
 * so that you can insert perl code -- hopefully not very much
 * -- into this loop.
 *
 *  The idea is to keep all the speed of pure C with the ability
 *  to do some organization and data prep/gathering in perl.
 */
void *dsp_thread_fn(SV *cfg, dp_baselist_t *bl) {
 uint32_t buffer_count = 0;
 time_t start = time(0);
 int done = 0;
 printf("-ci- dsp thread starting\n");
 dp_baselist_prerun(bl);
 printf("-ci- module pre-run complete\n");
 unsigned int runmask = 0;

 int rmask_idx = dp_pl_sharelist_get_idx(sl,"runmask");
 int done_idx  = dp_pl_sharelist_get_idx(sl,"dsp_done");

 while (!done) {
  runmask = dp_pl_sharelist_get_u32(sl,rmask_idx);
  dp_baselist_run(bl,runmask);

  done = dp_pl_sharelist_get_u32(sl,done_idx);
  /* printf("-cd- iter %d\n",buffer_count); */
  buffer_count++;
  dsp_after_cb_caller(cfg);
 };

 dp_baselist_postrun(bl);
 printf("-ci- module post-run complete\n");
 printf("-ci- dsp thread exiting\n");
};


/* convert a c char* string to a perl string */
SV *cp_to_svstr(char *a) {
 SV *p = newSVpvn(a,strlen(a));
};

/* convert a peak_pts_t struct to a perh hash object */
SV *read_peak_pts_t(dp_pl_sharelist_t *sl, peak_pts_t *ppts) {
 HV *hash;
 uint32_t i;
 uint32_t sr, fr;
 hash = newHV();
 hv_stores(hash,"have_fft",newSViv(1));
 if (1) {
  ___PERL_INSERT_HASH_COPYING_ppts 
  AV *av;
  av = newAV();
  fr = dp_pl_sharelist_get_u32(sl,dp_pl_sharelist_get_idx(sl,"curr_freq"));
  sr = dp_pl_sharelist_get_u32(sl,dp_pl_sharelist_get_idx(sl,"curr_sample_rate"));
  for (i=0;i<ppts->actpts;i++) { 
   HV *h2;
   float f;
   h2 = newHV();
   hv_stores(h2,"index",newSViv(ppts->points[i]->bin));
   hv_stores(h2,"dB",newSVnv(ppts->points[i]->db));
   hv_stores(h2,"abs",newSVnv(ppts->points[i]->abs));
   f = (float)ppts->points[i]->bin / (float)ppts->length;
   f *= (float)sr;
   f -= 0.5 * (float)sr;
   f += (float)fr;
   hv_stores(h2,"f",newSVnv(f));
   av_push(av,newRV_noinc((SV *)h2));
  }
  hv_stores(hash,"points",newRV_noinc((SV *)av));
 }
 return newRV_noinc((SV *)hash);
}


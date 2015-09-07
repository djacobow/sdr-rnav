#include "dp_iq_w.h"

DP_SUB_CREATOR_IMPL(iq_w)
DP_SUB_GENERIC_SETTER_IMPL(iq_w, num_channels, uint16_t)
DP_SUB_STRING_SETTER_IMPL(iq_w,fname)
DP_SUB_GENERIC_GETTER_IMPL(iq_w,last_elems,uint32_t)

void dp_iq_w_init(dp_base_t *b) {
 DP_IFSUBVALID(b,iq_w) {
  b->sub_work    = &dp_iq_w_work;
  b->sub_prerun  = &dp_iq_w_prerun;
  b->sub_postrun = &dp_iq_w_postrun;
  b->sub_deinit  = &dp_iq_w_deinit;
  s->num_channels = 1;
 }
}

void dp_iq_w_deinit(dp_base_t *b) {
 dp_iq_w_sub_t *s;
 if (DP_VALID(b)) {
  s = b->sub;
  if (s->isopen) {
   dp_iq_w_postrun(b);
  }
  if (s->fname) {
   free(s->fname);
  }
  b->sub_valid = 0;
 }
}

void
dp_iq_w_prerun(dp_base_t *b) {

 DP_IFSUBVALID(b,iq_w) {
#ifdef DEBUG
  fprintf(stderr,"-ci- iq_w (%s) opening %s\n",b->name,s->fname);
#endif
  s->fh = fopen(s->fname,"wb");
  if (!s->fh) {
   s->ec = 0x1; 
  }
  fprintf(s->fh,"IQ data file. %d channels of floating point data (%d bytes per sample follows",s->num_channels,4);
  s->isopen = 1;
 }
}


void
dp_iq_w_work(dp_base_t *b) {
 uint32_t written = 0;
 uint32_t count;
 uint32_t i, j;
 dp_int_t *data;
 DP_IFSUBVALID(b,iq_w) {
  count = b->runlength;
  if (!s->ec) { 
   data  = b->in_v->v;
   for (i=0;i<count;i++) {
    for (j=0;j<s->num_channels;j++) {
     float fv = (float)b->in_v->v[s->num_channels * i + j];
     written += fwrite(&fv, 4, 1, s->fh);
    }
   }
   s->bytes_written += written;
   s->last_elems = written/s->num_channels;
  }
 }
}

void
dp_iq_w_postrun(dp_base_t *b) {
 dp_iq_w_close(b);
}

void
dp_iq_w_close(dp_base_t *b) {
 DP_IFSUBVALID(b,iq_w) {
  if (s->isopen) {
   fclose(s->fh);
  }
  s->isopen = 0;
 }
}

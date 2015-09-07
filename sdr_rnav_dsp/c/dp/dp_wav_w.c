#include "dp_wav_w.h"

DP_SUB_CREATOR_IMPL(wav_w)
DP_SUB_GENERIC_SETTER_IMPL(wav_w, sample_rate, uint32_t)
DP_SUB_GENERIC_SETTER_IMPL(wav_w, num_channels, uint16_t)
DP_SUB_GENERIC_SETTER_IMPL(wav_w, bits_per_sample, uint16_t)
DP_SUB_STRING_SETTER_IMPL(wav_w,fname)
DP_SUB_GENERIC_GETTER_IMPL(wav_w,last_elems,uint32_t)

void dp_wav_w_init(dp_base_t *b) {
 DP_IFSUBVALID(b,wav_w) {
  b->sub_work    = &dp_wav_w_work;
  b->sub_prerun  = &dp_wav_w_prerun;
  b->sub_postrun = &dp_wav_w_postrun;
  b->sub_deinit  = &dp_wav_w_deinit;
  s->num_channels = 1;
  s->sample_rate  = 44100;
  s->bits_per_sample = 16;
 }
}

void dp_wav_w_deinit(dp_base_t *b) {
 dp_wav_w_sub_t *s;
 if (DP_VALID(b)) {
  s = b->sub;
  if (s->isopen) {
   dp_wav_w_postrun(b);
  }
  if (s->fname) {
   free(s->fname);
  }
  b->sub_valid = 0;
 }
}

void
dp_wav_w_prerun(dp_base_t *b) {
 uint32_t val32 = 0;
 uint16_t val16 = 1;
 uint32_t byte_rate;
 uint16_t blockalign;

 DP_IFSUBVALID(b,wav_w) {
#ifdef DEBUG
  fprintf(stderr,"-ci- wav_w (%s) opening %s\n",b->name,s->fname);
#endif
  s->fh = fopen(s->fname,"wb");
  if (!s->fh) {
   s->ec = 0x1; 
  }

  fwrite("RIFF",1,4,s->fh);
  fwrite(&val32,4,1,s->fh); /* will revisit at close */
  fwrite("WAVE",1,4,s->fh);
  fwrite("fmt\x20",1,4,s->fh);
  val32 = 16;
  fwrite(&val32,4,1,s->fh);
  fwrite(&val16,2,1,s->fh);
  fwrite(&(s->num_channels),2,1,s->fh);
  fwrite(&(s->sample_rate),4,1,s->fh);
  byte_rate = (s->bits_per_sample / 8) * s->sample_rate * s->num_channels;
  fwrite(&byte_rate,4,1,s->fh);
  blockalign = s->num_channels * (s->bits_per_sample / 8);
  fwrite(&blockalign,2,1,s->fh);
  fwrite(&(s->bits_per_sample),2,1,s->fh);
  fwrite("data",1,4,s->fh);
  val32 = 0;
  fwrite(&val32,4,1,s->fh);
  s->isopen = 1;
 }
}


void
dp_wav_w_work(dp_base_t *b) {
 uint32_t written;
 uint32_t count;
 dp_int_t *data;
 DP_IFSUBVALID(b,wav_w) {
  count = b->runlength;
  if (!s->ec) { 
   data  = b->in_v->v;
   switch (s->bits_per_sample) {
    case 8 :
     written = fwrite(data, 1, count*s->num_channels, s->fh); break;
    case 16 :
     written = fwrite(data, 2, count*s->num_channels, s->fh); break;
    case 32 :
     written = fwrite(data, 4, count*s->num_channels, s->fh); break;
    default:
     written = 0;
   }
   s->bytes_written += written * (s->bits_per_sample / 8);
   s->last_elems = written/s->num_channels;
  }
 }
}

void
dp_wav_w_postrun(dp_base_t *b) {
 dp_wav_w_close(b);
}

void
dp_wav_w_close(dp_base_t *b) {
 DP_IFSUBVALID(b,wav_w) {
  if (s->isopen) {
   uint32_t val;
   fseek(s->fh,4,SEEK_SET);
   val = s->bytes_written + 44 - 8;
   fwrite(&val,4,1,s->fh);
   fseek(s->fh,40,SEEK_SET);
   val = s->bytes_written;
   fwrite(&val,4,1,s->fh);
   fclose(s->fh);
  }
  s->isopen = 0;
 }
}

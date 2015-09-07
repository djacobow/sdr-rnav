/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz

// A class to provide quick and dirty read access to Microsoft ".wav" 
// files. 
//
// How is this quick and dirty:
//  1. almost no error checking
//  2. works on little-endian machines only. That's x86 and ARM, but 
//  if you're porting you'll probably get bitten.
//  2. the code is full of magic numbers, etc. 
*/

#include "dp_wav_r.h"
#include <stdio.h>
#include <stdlib.h>

DP_SUB_CREATOR_IMPL(wav_r)
DP_SUB_STRING_SETTER_IMPL(wav_r,fname)
DP_SUB_GENERIC_GETTER_IMPL(wav_r,sample_rate,uint32_t)
DP_SUB_GENERIC_GETTER_IMPL(wav_r,num_channels,uint8_t)
DP_SUB_GENERIC_GETTER_IMPL(wav_r,sample_size,uint8_t)
DP_SUB_GENERIC_GETTER_IMPL(wav_r,last_elems,uint32_t)

void
dp_wav_r_init(dp_base_t *b) {
 if (DP_SUBVALID(b,wav_r)) {
  b->sub_work    = &dp_wav_r_work;
  b->sub_prerun  = &dp_wav_r_prerun;
  b->sub_deinit  = &dp_wav_r_deinit;
 }
}


void dp_wav_r_deinit(dp_base_t *b) {
 dp_wav_r_sub_t *s;
 if (DP_VALID(b)) {
  s = b->sub;
  if (s->fname) {
   free(s->fname);
  }
  if (s->isopen) {
   fclose(s->fh);
  }
  b->sub_valid = 0;
 }
}



uint8_t match_chars(char *a, const char *b, uint8_t l) {
 uint8_t i =0;
 uint8_t miss = 0;
 for (i=0;i<l;i++) {
  if (a[i] != b[i]) {
   miss += 1;
  }
 }
 return miss;
}

void dp_wav_r_prerun(dp_base_t *b) {
 char riff[4];
 uint32_t chunksize = 0;
 char wave[4];
 dp_wav_r_sub_t *s;
 char fmt[4];
 uint32_t subchunksize = 0;
 uint32_t fmt_subchunk_pos;
 uint32_t byterate = 0;
 char dmy;
 char     data[4];

 if (DP_SUBVALID(b,wav_r)) {
  s = b->sub;
  s->fh = fopen(s->fname, "rb");
  if (s->fh) {
   s->isopen = 1;
   s->pos = 0;
   s->iseof = 0;

   /* Header */
   s->pos = s->pos + 1 * fread(riff,1,4,s->fh);
   if (match_chars(riff,"RIFF",4)) { s->ec |= 0x1; }
   s->pos = s->pos + 4 * fread(&chunksize,4,1,s->fh);
   s->pos = s->pos + 1 * fread(wave,1,4,s->fh);
   if (match_chars(wave,"WAVE",4)) { s->ec |= 0x2; }

   /* descriptor chunk */
   s->pos = s->pos + 1 * fread(fmt,1,4,s->fh);
   if (match_chars(fmt,"fmt",3)) { s->ec |= 0x4; }
   s->pos = s->pos + 4 * fread(&subchunksize,4,1,s->fh);
   fmt_subchunk_pos = s->pos;
   s->pos = s->pos + 2 * fread(&(s->audioformat),2,1,s->fh);
   if (s->audioformat != 1) { s->ec |= 0x8; }
   s->pos = s->pos + 2 * fread(&(s->num_channels),2,1,s->fh); 
   s->pos = s->pos + 4 * fread(&(s->sample_rate),4,1,s->fh);
   s->pos = s->pos + 4 * fread(&byterate,4,1,s->fh);
   s->pos = s->pos + 2 * fread(&(s->blockalign),2,1,s->fh);
   s->pos = s->pos + 2 * fread(&(s->sample_size),2,1,s->fh);

   /* the first subchunk is normally 16B but apparently some 
   // converters pad it, so we eat up bytes until we are where
   // we expect the "data" to be.
   */
   if ((s->pos - fmt_subchunk_pos) < subchunksize) {
    while ((s->pos - fmt_subchunk_pos) < subchunksize) {
    s->pos += 1 * fread(&dmy,1,1,s->fh);
    }
   }
   /* data */
   s->pos = s->pos + 1 * fread(data,1,4,s->fh);
   if (match_chars(data,"data",3)) { s->ec |= 0x10; }
   s->subchunk2size = 0;
   s->pos = s->pos + 4 * fread(&(s->subchunk2size),4,1,s->fh);
   s->goodfile = 1;
#ifdef DEBUG
   fprintf(stderr,"-ci- wav_r (%s) opened file %s\n",
		   b->name,s->fname);
#endif
   s->iseof = feof(s->fh);
   return;
  } else {
   s->ec |= 0x20;
  }
 }
}

void
dp_wav_r_work(dp_base_t *b) {
 dp_wav_r_sub_t *s = b->sub;
 uint32_t actual = 0;
 uint32_t count = b->runlength;

 int16_t *d = b->out_v->v;
 if (s->ec) {
  x1(s->ec); exit(-1);
 }
 switch (s->sample_size) {
  case 8 :
   actual = fread(d, 1, count*s->num_channels, s->fh); break;
  case 16 :
   actual = fread(d, 2, count*s->num_channels, s->fh); break;
  case 32 :
   actual = fread(d, 4, count*s->num_channels, s->fh); break;
  default :
   actual = 0;
 }

 if (feof(s->fh)) {
  s->iseof = 1;
 }
 s->total_data_bytes += actual * s->num_channels * (s->sample_size / 8);
 s->last_elems = actual / s->num_channels;
}

void dp_wav_r_postrun(dp_base_t *b) {
}

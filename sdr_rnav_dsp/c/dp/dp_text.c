/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz

// A debugging class for writing a .csv file. Useful for looking at 
// intermediate results in Excel.
*/

#include "dp_text.h"
#include <stdio.h>

DP_SUB_CREATOR_IMPL(text)
DP_SUB_STRING_SETTER_IMPL(text,fname)
DP_SUB_STRING_SETTER_IMPL(text,header)
DP_SUB_GENERIC_SETTER_IMPL(text,channels,uint8_t)

DP_FN_PREAMBLE(text,init) {
 b->sub_work = &dp_text_work;
 b->sub_deinit = &dp_text_deinit;
 b->sub_prerun = &dp_text_prerun;
 b->sub_postrun = &dp_text_postrun;
};
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(text,deinit) {
 dp_text_postrun(b);
 if (s->fname)  { free(s->fname); }
 if (s->header) { free(s->header); }
}
DP_FN_POSTAMBLE

DP_FN_PREAMBLE(text,prerun) {
 s->fh = fopen(s->fname,"w");
 fprintf(s->fh,"iter,i,%s\n",s->header);
 if (s->fh && !ferror(s->fh)) {
  fprintf(stderr,"-ci- text (%s) successfully opened file %s\n",b->name,s->fname);
  return;
 } 
 s->ec |= 1;
 return;
};
DP_FN_POSTAMBLE


DP_FN_PREAMBLE(text,work) {
 uint32_t i,j;
 for (i=0;i<b->runlength;i++) {
  fprintf(s->fh,"%d,%d,",s->iter,i);
  for (j=0;j<s->channels;j++) {
   fprintf(s->fh,"%d,",b->in_v->v[s->channels*i+j]);
  }
  fputs("\n",s->fh);
 }
 s->iter++;
}
DP_FN_POSTAMBLE


void
dp_text_iput(dp_base_t *b, const int16_t *buf, uint32_t len, uint8_t channels) {
 DP_IFSUBVALID(b,text) {
  uint32_t i,j;
  for (i=0;i<len;i++) {
   for (j=0;j<channels;j++) {
    fprintf(s->fh,"%d,",buf[channels*i+j]);
   }
   fputs("\n",s->fh);
  }
 }
}

void
dp_text_fput(dp_base_t *b, const float *buf, uint32_t len, uint8_t channels) {
 DP_IFSUBVALID(b,text) {
  uint32_t i,j;
  for (i=0;i<len;i++) {
   for (j=0;j<channels;j++) {
    fprintf(s->fh,"%f,",buf[channels*i+j]);
   }
   fputs("\n",s->fh);
  }
 }
}
void
dp_text_bput(dp_base_t *b, const dp_bool_t *buf, uint32_t len, uint8_t channels) {
 DP_IFSUBVALID(b,text) {
  uint32_t i,j;
  for (i=0;i<len;i++) {
   for (j=0;j<channels;j++) {
    fprintf(s->fh,"%d,",buf[channels*i+j] ? 1 : 0);
   }
   fputs("\n",s->fh);
  }
 }
}


DP_FN_PREAMBLE(text,postrun) {
 if (s->isopen) { fclose(s->fh); };
}
DP_FN_POSTAMBLE


// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz

// A debugging class for writing a .csv file. Useful for looking at 
// intermediate results in Excel.

#include "text_dump.h"

namespace djdsp {

text_dump_c::text_dump_c() {
 isopen = false;
 fname.clear();
 header.clear();
 ec = 0;
 iter = 0;
};

void
text_dump_c::set_file(const std::string fn, const std::string hdr) {
 fname = fn;
 header = hdr;
}

void
text_dump_c::pre_run() {
 const char *fn = fname.c_str();
 const char *hdr = header.c_str();
 fh = fopen(fn,"w");
 fprintf(fh,"iter,i,%s\n",hdr);
 if (fh && !ferror(fh)) {
  return;
 } 
 ec |= 1;
 return;
};

void
text_dump_c::set_channels(uint8_t ch) {
 channels = ch;
}

void
text_dump_c::work() {
 for (uint32_t i=0;i<l;i++) {
  fprintf(fh,"%d,%d,",iter,i);
  for (uint8_t j=0;j<channels;j++) {
   fprintf(fh,"%d,",(*in)[channels*i+j]);
  }
  fputs("\n",fh);
 }
 iter++;
}


void
text_dump_c::iput(const int16_t *buf, uint32_t len, uint8_t channels) {
 for (uint32_t i=0;i<len;i++) {
  for (uint8_t j=0;j<channels;j++) {
   fprintf(fh,"%d,",buf[channels*i+j]);
  }
  fputs("\n",fh);
 }
};

void
text_dump_c::fput(const float *buf, uint32_t len, uint8_t channels) {
 for (uint32_t i=0;i<len;i++) {
  for (uint8_t j=0;j<channels;j++) {
   fprintf(fh,"%f,",buf[channels*i+j]);
  }
  fputs("\n",fh);
 }
};

void
text_dump_c::bput(const bool *buf, uint32_t len, uint8_t channels) {
 for (uint32_t i=0;i<len;i++) {
  for (uint8_t j=0;j<channels;j++) {
   fprintf(fh,"%d,",buf[channels*i+j] ? 1 : 0);
  }
  fputs("\n",fh);
 }
};

void
text_dump_c::close() {
 if (isopen) { fclose(fh); };
}

text_dump_c::~text_dump_c() {
 close();
}



} // namespace

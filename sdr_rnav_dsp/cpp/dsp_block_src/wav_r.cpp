// Author: David Jacobowitz
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

#include "wav_r.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

namespace djdsp {

wav_r_c::wav_r_c() {
 valid = 1;
 goodfile = 0;
 isopen = 0;
 iseof = 0;
 pos = 0;
 fh = 0;
 ec = 0;
 total_data_bytes = 0;
 last_elems = 0;
 fname.clear();
};

uint8_t match_chars(char *a, const char *b, uint8_t l) {
 uint8_t i = 0;
 uint8_t miss = 0;
 for(i=0;i<l;i++) {
  if (a[i] != b[i]) {
   miss += 1;
  }
 }
 return miss;
};

uint32_t wav_r_c::get_sample_rate()  { return sample_rate; };
uint16_t wav_r_c::get_num_channels() { return num_channels; };
uint16_t wav_r_c::get_sample_size()  { return sample_size; };
uint16_t wav_r_c::get_format()       { return audioformat; };

void
wav_r_c::pre_run() {

 const char *fn = fname.c_str();

 if (valid) {
  fh = fopen(fn, "rb");
  if (fh) {
   isopen = 1;
   pos = 0;
   iseof = 0;

   /* Header */
   char riff[4];
   pos = pos + 1 * fread(riff,1,4,fh);
   if (match_chars(riff,"RIFF",4)) { ec |= 0x1; }
   uint32_t chunksize = 0;
   pos = pos + 4 * fread(&chunksize,4,1,fh);
   char wave[4];
   pos = pos + 1 * fread(wave,1,4,fh);
   if (match_chars(wave,"WAVE",4)) { ec |= 0x2; }

   /* descriptor chunk */
   char fmt[4];
   pos = pos + 1 * fread(fmt,1,4,fh);
   if (match_chars(fmt,"fmt",3)) { ec |= 0x4; }
   uint32_t subchunksize = 0;
   pos = pos + 4 * fread(&subchunksize,4,1,fh);
   uint32_t fmt_subchunk_pos = pos;
   pos = pos + 2 * fread(&audioformat,2,1,fh);
   if (audioformat != 1) { ec |= 0x8; }
   pos = pos + 2 * fread(&num_channels,2,1,fh); 
   pos = pos + 4 * fread(&sample_rate,4,1,fh);
   uint32_t byterate = 0;
   pos = pos + 4 * fread(&byterate,4,1,fh);
   pos = pos + 2 * fread(&blockalign,2,1,fh);
   pos = pos + 2 * fread(&sample_size,2,1,fh);

   // the first subchunk is normally 16B but apparently some 
   // converters pad it, so we eat up bytes until we are where
   // we expect the "data" to be.
   char dmy;
   if ((pos - fmt_subchunk_pos) < subchunksize) {
    while ((pos - fmt_subchunk_pos) < subchunksize) {
    pos += 1 * fread(&dmy,1,1,fh);
    }
   }
   /* data */
   char     data[4];
   pos = pos + 1 * fread(data,1,4,fh);
   if (match_chars(data,"data",3)) { ec |= 0x10; }
   subchunk2size = 0;
   pos = pos + 4 * fread(&subchunk2size,4,1,fh);
   goodfile = 1;
#ifdef DEBUG
   std::cout << "-info- wav_r (" << name << ") opened " << fname 
	     << std::endl;
#endif
   iseof = feof(fh);
   return;
  } else {
   ec |= 0x20;
  }
 }
 ec |= 0x40; 
};

void wav_r_c::set_file(const std::string ifn) {
 fname = ifn;
};

void wav_r_c::file_info_str() {
 std::cout << "wav file data:" << std::endl
           << " struct valid : " << valid        << std::endl
           << " file open    : " << isopen       << std::endl
           << " file good    : " << goodfile     << std::endl
           << " curr pos     : " << pos          << std::endl
           << " samp rate    : " << sample_rate  << std::endl
           << " num chanels  : " << num_channels << std::endl
           << " samp size    : " << sample_size  << std::endl
           << " block align  : " << blockalign   << std::endl
           << " data size    : " << subchunk2size << std::endl;
};

void
wav_r_c::close() {
 if (valid && isopen) {
  fclose(fh);
  isopen = 0;
 }
};

wav_r_c::~wav_r_c() {
 close();
}

void
wav_r_c::work() {
 uint32_t actual = 0;
 uint32_t count = l;

 int16_t *d = out->data();
 if (ec) {
  d1(ec); exit(-1);
 }
 switch (sample_size) {
  case 8 :
   actual = fread(d, 1, count*num_channels, fh); break;
  case 16 :
   actual = fread(d, 2, count*num_channels, fh); break;
  case 32 :
   actual = fread(d, 4, count*num_channels, fh); break;
  default :
   actual = 0;
 }

 if (feof(fh)) {
  iseof = 1;
 }
 total_data_bytes += actual * num_channels * (sample_size / 8);
 // d2(count,actual);
 // printf("bytes read after %8.8x\n", total_data_bytes);
 last_elems = actual / num_channels;
 // d2(count,last_elems);
};

uint32_t 
wav_r_c::lastElems() {
 return last_elems;
};


} // namespace

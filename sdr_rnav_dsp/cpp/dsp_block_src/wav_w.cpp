// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : 5/7/2013
//
// Copyright 2013, David Jacobowitz

#include <iostream>
#include "wav_w.h"

namespace djdsp {

// A class to write simple ".wav" files. It's not pretty. It's very
// quick and dirty  and also by the way totally not endian aware!
// Can write mono or stereo wav files of byte, word, or dword elements
// at any sample rate.

wav_w_c::wav_w_c() {
 isopen = false;
 num_channels = 1;
 sample_rate = 8000;
 bits_per_sample = 16;
 bytes_written = 0;
 ec = 0;
 last_elems = 0;
 fname.clear();
};

void wav_w_c::set_sample_rate(uint32_t sr) { sample_rate = sr; };
void wav_w_c::set_num_channels(uint8_t nc) { num_channels = nc; };
void wav_w_c::set_bits_per_sample(uint16_t bits) { bits_per_sample = bits; };
void wav_w_c::set_file(const std::string fn) { fname = fn; }	

void
wav_w_c::pre_run() {

 const char *fn = fname.c_str();
#ifdef DEBUG
 std::cout << "-info- wav_w (" << name << ") opening " << fname 
	   << std::endl;
#endif
 fh = fopen(fn,"wb");
 if (!fh) {
  ec = 0x1; 
 }

 fwrite("RIFF",1,4,fh);
 uint32_t val32 = 0;
 fwrite(&val32,4,1,fh); // will revisit at close
 fwrite("WAVE",1,4,fh);
 fwrite("fmt\x20",1,4,fh);
 val32 = 16;
 fwrite(&val32,4,1,fh);
 uint16_t val16 = 1;
 fwrite(&val16,2,1,fh);
 fwrite(&num_channels,2,1,fh);
 fwrite(&sample_rate,4,1,fh);
 uint32_t byte_rate = (bits_per_sample / 8) * sample_rate * num_channels;
 fwrite(&byte_rate,4,1,fh);
 uint16_t blockalign = num_channels * (bits_per_sample / 8);
 fwrite(&blockalign,2,1,fh);
 fwrite(&bits_per_sample,2,1,fh);
 fwrite("data",1,4,fh);
 val32 = 0;
 fwrite(&val32,4,1,fh);
 isopen = true;
};

void
wav_w_c::work() {

 uint32_t written;
 uint32_t count = l;

 // d1(in->size());
 // d2(ec,l);
 if (!ec) { 
  // show_dv(*in);
  const int16_t *d = in->data();
  switch (bits_per_sample) {
   case 8 :
    written = fwrite(d, 1, count*num_channels, fh); break;
   case 16 :
    written = fwrite(d, 2, count*num_channels, fh); break;
   case 32 :
    written = fwrite(d, 4, count*num_channels, fh); break;
   default:
    written = 0;
  };
  bytes_written += written * (bits_per_sample / 8);
  last_elems = written/num_channels;
 };
};

uint32_t 
wav_w_c::lastElems() {
 return last_elems;
};


void
wav_w_c::close() {
 if (isopen) {
  fseek(fh,4,SEEK_SET);
  // printf("bytes written %lx %ld\n",bytes_written, bytes_written);
  uint32_t val = bytes_written + 44 - 8;
  fwrite(&val,4,1,fh);
  fseek(fh,40,SEEK_SET);
  val = bytes_written;
  fwrite(&val,4,1,fh);
  fclose(fh);
 }
 isopen = false;
};

wav_w_c::~wav_w_c() {
 close();
};

} // namespace

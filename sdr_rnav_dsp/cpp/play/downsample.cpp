#include "wav_r.h"
#include "wav_w.h"
#include <iostream>
#include "decimate.h"

uint32_t rb_len = 16*1024;

int main(int argc, char *argv[]) {

 dvec_t read_buffer(2*rb_len);
 dvec_t write_buffer(rb_len);

 wav_r_c reader;
 reader.set_name("reader");
 reader.set_out(&read_buffer);
 reader.set_runlen(rb_len);
 reader.set_file("test_input_files/OAK_r324_20130429_162637Z_116782kHz_IQ.wav");

 decimate_c decimator;
 decimator.set_name("decimator");
 decimator.set_complex(true);
 decimator.set_runlen(rb_len);
 decimator.set_decim(2);
 decimator.set_in(&read_buffer);
 decimator.set_out(&write_buffer);

 wav_w_c writer;
 writer.set_name("writer");
 writer.set_num_channels(2);
 writer.set_bits_per_sample(16);
 writer.set_runlen(rb_len/2);
 writer.set_in(&write_buffer);
 writer.set_file("decimated.wav");

 bool done = false;
 int ct = 0;
 reader.pre_run();
 decimator.pre_run();
 reader.file_info_str();

 uint32_t sr = reader.get_sample_rate();
 uint32_t osr = sr/2;
 writer.set_sample_rate(osr);

 writer.pre_run();
 std::cout << "reader sample rate: " << sr << std::endl;
 std::cout << "writer sample rate: " << osr << std::endl;
 while (!done) {
  std::cout << "reader reading block" << std::endl;
  reader.work();
  std::cout << "reader worked" << std::endl;
  decimator.work();
  writer.work();
  done |= (reader.lastElems() == 0);
  d1(ct);
  ct++;
 }

 reader.post_run();
 decimator.post_run();
 writer.post_run();

 return 0;
}


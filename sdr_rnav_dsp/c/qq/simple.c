#include <stdlib.h>
#include <stdio.h>

#include "dp_base.h"
#include "dp_baselist.h"
#include "dp_radiofcdpp.h"
#include "dp_wav_w.h"

static const uint32_t read_runlen = 8192;

#define RUN_MASK_ALWAYS (0x1)


int main(int argc, char *argv[]) {
 int count = 0;
 pthread_t *thr;
 dp_vec_t *rd_buffer;

 dp_base_t *radio;
 dp_base_t *writer;

 rd_buffer = dp_vec_create(read_runlen*2);

 dp_baselist_t *bl = dp_baselist_create(3);
 
 DP_CREATE(radio,radiofcdpp);
 dp_set_name(radio,"theradio");
 dp_radiofcdpp_set_buffers(radio,read_runlen,4);
 dp_radiofcdpp_open_device(radio,"hw:CARD=V20,DEV=0");
 dp_set_out(radio,rd_buffer);
 dp_set_group(radio,RUN_MASK_ALWAYS);
 dp_baselist_add(bl, radio);

 DP_CREATE(writer,wav_w);
 dp_set_name(writer,"thewriter");
 dp_set_in(writer,rd_buffer);
 dp_set_group(writer,RUN_MASK_ALWAYS);
 dp_set_runlen(writer,read_runlen);
 dp_wav_w_set_fname(writer,"output.wav");
 dp_wav_w_set_bits_per_sample(writer,16);
 dp_wav_w_set_sample_rate(writer,192000);
 dp_wav_w_set_num_channels(writer,2);
 dp_baselist_add(bl, writer);

 thr = dp_radiofcdpp_start_thread(radio);

 dp_baselist_prerun(bl);

 while (count < 100) {
  dp_baselist_run(bl,RUN_MASK_ALWAYS); 
  count++;  
 }

 dp_baselist_postrun(bl);
 return 0;

};


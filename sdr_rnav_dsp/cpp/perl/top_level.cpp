
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013 
//
// Copyright 2013, David Jacobowitz

// wrapper functions for Inline Perl to acces the radio.
// There should not be too much interesting here.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <iostream>
#include <ctime>
#include <string.h>


#ifdef __linux
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#endif

#include "rtl_help.h"
#include "dsp_chain.h"
#include "my_console.h"

#ifdef __WIN32
const std::string idir = "..\\test_input_files\\";
const std::string odir = "..\\test_output_files\\";
#else
const std::string idir = "../test_input_files/";
const std::string odir = "../test_output_files/";
#endif

// These defaults will all correctly decode the OAK file listed below,
// and they can all be overriddent from the command line with getArgs()
int             _main_max_cap_seconds  = -1; // neg indicates forever
unsigned int    _main_freq             = 116800000;
bool            _main_use_mixer        = true;
bool            _main_perform_fft      = false;
float           _main_mixer_lo_freq    = 15000;
float           _main_radial_calibrate = -175;
std::string     _main_infile           = idir + "OAK_r324_20130429_162637Z_116782kHz_250000sps_IQ.wav";
float           _main_min_snr          = 10.0;
int             _main_port             = 4044;

djdsp::rtl_help_c radio;
receiver_stat_t lrstat;
djdsp::peak_pts_t pts;
std::string id_instr = "";
bool have_status = false;
bool have_fft    = false;

#ifdef __linux
void my_handler(int sig) {
 void *array[10];
 size_t size;
 size = backtrace(array, 100);
 fprintf(stderr, "Error: signal %d\n", sig);
 backtrace_symbols_fd(array, size, STDERR_FILENO);
 exit(1);
}
#else 
// This is only necessary because of a bug in Perl Inline::CPP
void my_handler(int sig) { };
#endif


int radio_init() {

#ifdef _WIN32
// This turns off the error dialog that Windows throws up. 
// Just a time saver when you're debugging a crashy program.
DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif

#ifdef __linux
 signal(SIGSEGV, my_handler);
 signal(SIGABRT, my_handler);
#endif

 // initialize and open the rtlsdr device. 
 radio.set_name("radio");
 int dev_count = radio.list_rtl_devices();
 int index     = 0;
 if (index >= dev_count) {
  std::cerr << "-err-- no such device index" << std::endl;;
  exit(-1);
 }
 radio.open_rtl_device(index);

 pthread_t dsp_thread;
 pthread_t rtl_thread;

 pthread_create(&dsp_thread,NULL,dsp_thread_fn,&radio);
 pthread_create(&rtl_thread,NULL,rtl_thread_fn,&radio);

 my_sleep(100);
}

int radio_shutdown() {
 _dsp_chain_all_done = true; 
 int rv = radio.close_rtl_device();
 return rv;
};

int radio_set_gain(unsigned int g) {
 int rv = radio.dev_cmd(djdsp::ST_GAIN,g);
 std::cout << "-info- (ctrl) NEW GAIN: " << g << std::endl;
 return rv;
}

int radio_enable_agc(bool b) {
 int rv = radio.dev_cmd(djdsp::ST_AGC,b);
 std::cout << "-info- (ctrl) NEW AGC mode: " << b << std::endl;
 return rv;
}

int radio_enable_test_mode(bool b) {
 int rv = radio.dev_cmd(djdsp::ST_TEST,b);
 std::cout << "-info- (ctrl) NEW TEST mode: " << b << std::endl;
 return rv;
};

int radio_set_sample_rate(unsigned int sr) {
 int rv = radio.dev_cmd(djdsp::ST_SR,sr);
 std::cout << "-info- (ctrl) NEW sample rate: " << sr << std::endl;
 return rv;
};

int radio_set_frequency(unsigned int f) {
 int rv = radio.dev_cmd(djdsp::ST_FREQ,f);
 _main_freq = f;
 std::cout << "-info- (ctrl) NEW FREQ: " << f << std::endl;
 return rv;
};

int radio_enable_mixer(int m) {
 _main_use_mixer = m != 0;
};

int radio_set_mixer_lo(uint32_t lo) {
 _main_mixer_lo_freq = lo;
 bool lo_impossible = lrstat.mixer->set_lo_freq(_main_mixer_lo_freq);
 if (!lo_impossible) {
  std::cout << "-info- (ctrl) adjusting lo to: " << _main_mixer_lo_freq << std::endl;
 } else {
  std::cout << "-warn- (ctrl) LO freq too low. Turning off mixer." << std::endl;
  _main_use_mixer = false;
 }
};

SV *radio_get_status() {
 have_status = _dsp_chain_rstat_queue.try_pop_all(lrstat);

 HV *hash;
 hash = newHV();
 hv_stores(hash,"have_status",newSViv(have_status));
 if (have_status) {
 ___PERL_INSERT_HASH_COPYING_lrstat
  std::string a_bit = "";
  while (_dsp_chain_id_text_queue.try_pop(a_bit)) {
   id_instr += a_bit;
  }
  hv_stores(hash,"id_instr",newSVpv(id_instr.c_str(),id_instr.size()));
  id_instr.clear();
 }

 return newRV_noinc((SV *)hash);
};

bool radio_enable_fft(int f) {
 _main_perform_fft = f != 0;
};

SV *radio_get_fft() {
 have_fft = _dsp_chain_peaks_queue.try_pop_all(pts);
 HV *hash;
 hash = newHV();
 hv_stores(hash,"have_fft",newSViv(have_fft));
 if (have_fft) {
  ___PERL_INSERT_HASH_COPYING_pts 
  AV *av;
  av = newAV();
  for (uint32_t i=0;i<pts.points.size();i++) { 
   HV *h2;
   h2 = newHV();
   hv_stores(h2,"index",newSViv(pts.points[i].bin));
   hv_stores(h2,"dB",newSVnv(pts.points[i].db));
   av_push(av,newRV_noinc((SV *)h2));
  }
  hv_stores(hash,"points",newRV_noinc((SV *)av));
 }
 return newRV_noinc((SV *)hash);
};


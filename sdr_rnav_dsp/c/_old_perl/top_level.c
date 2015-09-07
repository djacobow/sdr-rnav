
/* Author: David Jacobowitz
           david.jacobowitz@gmail.com
  
   Date  : Summer 2013 
  
   Copyright 2013, David Jacobowitz
*/

/* wrapper functions for Inline Perl to acces the radio.
   There should not be too much interesting here.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>


#ifdef __linux
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#endif

#include "dp_radio2832.h"
#include "dsp_chain.h"
/* #include "my_console.h" */
#include "my_qs.h"


/* These defaults will all correctly decode the OAK file listed below,
// and they can all be overridden from the command line with getArgs()
*/
int             _main_max_cap_seconds  = -1; // neg indicates forever
unsigned int    _main_freq             = 116800000;
unsigned int    _main_sample_rate      = 250000;
dp_bool_t       _main_use_mixer        = true;
dp_bool_t       _main_perform_fft      = false;
float           _main_mixer_lo_freq    = 15000;
float           _main_radial_calibrate = 0;
char            _main_infile[MAX_INFILE_LEN];
char            _main_odir[MAX_INFILE_LEN];
float           _main_min_snr          = 10.0;
float           _main_fft_pk_thresh    = 10.0;
int             _main_ctrl_has_begun   = 0;
int             _main_decode_has_begun = 0;

dp_base_t         *radio;
receiver_stat_t   lrstat;
peak_pts_t        pts;
char id_instr[80];
dp_bool_t have_status = false;
dp_bool_t have_fft    = false;
pthread_t dsp_thread, rtl_thread;

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
/* This is only necessary because of a bug in Perl Inline::CPP */
void my_handler(int sig) { }
#endif


int radio_init(int index) {

 int dev_count;
 int rv;
#ifdef _WIN32
/* This turns off the error dialog that Windows throws up. 
   Just a time saver when you're debugging a crashy program. */
DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif

#ifdef __linux
 signal(SIGSEGV, my_handler);
 signal(SIGABRT, my_handler);
#endif

 _main_ctrl_has_begun = 0;
 _main_decode_has_begun = 0;

 memset(id_instr,0,80);
 memset(_main_infile,0,MAX_INFILE_LEN);
 memset(_main_odir,0,MAX_INFILE_LEN);
#ifdef _WIN32
 strncpy(_main_odir,"..\\..\\test_output_files\\",MAX_INFILE_LEN);
#endif
#ifdef __linux
 strncpy(_main_odir,"../../test_output_files/",MAX_INFILE_LEN);
#endif

 DP_CREATE(radio,radio2832);
 dp_set_name(radio,"radio");

#ifdef USE_RADIO
 /* initialize and open the rtlsdr device.  */
 dev_count = dp_radio2832_list_devices(radio);
 index     = 0;
 if (index >= dev_count) {
  fprintf(stderr,"-err-- no such device index: %d\n",index);
  exit(-1);
 }
 rv = dp_radio2832_open_device(radio,index);
 if (rv) {
  fprintf(stderr,"-err-- could not open device\n");
 }

 /* my_sleep(1000); */

 pthread_create(&rtl_thread,NULL,rtl2832_thread_fn,radio);
 if (rv) {
  fprintf(stderr,"-err-- could not create rtl thread\n");
 }
#endif

 /* my_sleep(1000); */
 printf("-info- radio thread started\n");
 _main_ctrl_has_begun |= 1;
}

void radio_calibrate_radial(float cr) {
 _main_radial_calibrate = cr;
};

void radio_preset_outdir(SV *ofd) {
 char *n = SvPV(ofd,PL_na);
 strncpy(_main_infile,n,MAX_INFILE_LEN);
};

void radio_preset_infile(SV *ifn) {
 char *n = SvPV(ifn,PL_na);
 strncpy(_main_infile,n,MAX_INFILE_LEN);
};

void radio_preset_peaks_thresh(float t) {
 _main_fft_pk_thresh = t;
};

void radio_preset_max_cap_seconds(int m) {
 _main_max_cap_seconds = m;
}

int radio_dsp_all_done() {
 return _dsp_chain_all_done;
};

void radio_dsp_init() {
 int rv;
 rv = pthread_create(&dsp_thread,NULL,dsp_thread_fn,radio); 
 if (rv) {
  fprintf(stderr,"-err-- could not create dsp thread\n");
 } else {
  fprintf(stderr,"-info- dsp thread created\n");
 }
}


int radio_shutdown() {
 _dsp_chain_all_done = true; 
 return(dp_radio2832_close_device(radio));
}

int radio_set_tuner_gain_mode(unsigned int g) {
 int rv = dp_radio2832_dev_cmd(radio,ST_TUNER_GAIN_MODE,&g);
 fprintf(stderr,"-info- (ctrl) NEW GAIN MODE: %d\n",g);
 return rv;
}
int radio_set_gain(unsigned int g) {
 int rv = dp_radio2832_dev_cmd(radio,ST_TUNER_GAIN,&g);
 fprintf(stderr,"-info- (ctrl) NEW GAIN: %d\n",g);
 return rv;
}

int radio_enable_agc(unsigned int b) {
 int rv = dp_radio2832_dev_cmd(radio,ST_AGC,&b);
 fprintf(stderr,"-info- (ctrl) NEW AGC mode: %d\n",b);
 return rv;
}

int radio_enable_test_mode(unsigned int b) {
 int rv = dp_radio2832_dev_cmd(radio,ST_TEST,&b);
 fprintf(stderr,"-info- (ctrl) NEW TEST mode: %d\n",b);
 return rv;
}

int radio_set_sample_rate(unsigned int sr) {
 int rv = dp_radio2832_dev_cmd(radio,ST_SR,&sr);
 _main_sample_rate = sr;
 fprintf(stderr,"-info- (ctrl) NEW sample rate: %d\n",sr);
 return rv;
}

int radio_set_frequency(unsigned int nf) {
 int rv = dp_radio2832_dev_cmd(radio,ST_FREQ,&nf);
 _main_freq = nf;
 fprintf(stderr,"-info- (ctrl) NEW freq: %d\n",nf);
 return rv;
}

int radio_enable_mixer(int m) {
 _main_use_mixer = m != 0;
}

int radio_set_mixer_lo(unsigned int lo) {
 dp_bool_t lo_impossible = dp_cmix_set_lo_freq(lrstat.mixer,_main_mixer_lo_freq);
 _main_mixer_lo_freq = lo;

 if (!lo_impossible) {
  fprintf(stderr, "-info- (ctrl) adjusting lo to: %d\n",_main_mixer_lo_freq);
 } else {
  fprintf(stderr,"-warn- (ctrl) LO freq too low. Turning off mixer.\n");
  _main_use_mixer = false;
 }
}

SV *radio_get_status() {
 have_status = dp_conc_q_rstat_try_pop_all(&_dsp_chain_rstat_queue,&lrstat);

 float angle, angle_lpf;
 HV *hash;
 hash = newHV();
 hv_stores(hash,"have_status",newSViv(have_status));
 if (have_status) {
  ___PERL_INSERT_HASH_COPYING_lrstat
  angle     = 30.0 * lrstat.phase_diff;
  angle_lpf = 30.0 * lrstat.phase_diff_lpf;
  angle     *= 360.0;
  angle_lpf *= 360.0;
  angle     += _main_radial_calibrate;
  angle_lpf += _main_radial_calibrate;
  angle      = (angle     < 0)   ? angle     + 360.0 : 
	       (angle     > 360) ? angle     - 360.0 : angle;
  angle_lpf  = (angle_lpf < 0)   ? angle_lpf + 360.0 : 
	       (angle_lpf > 360) ? angle_lpf - 360.0 : angle_lpf;

  hv_stores(hash,"angle",    newSVnv(angle));
  hv_stores(hash,"angle_lpf",newSVnv(angle_lpf));
  char a_bit = 0;
  id_instr[0] = 0;
  while (dp_conc_q_char_try_pop(&_dsp_chain_id_text_queue, &a_bit)) {
   int l = strlen(id_instr);
   id_instr[l] = a_bit;
   id_instr[l+1] = 0;
  }
  hv_stores(hash,"id_instr",newSVpv(id_instr,strlen(id_instr)));
  id_instr[0] = 0;
 }

 return newRV_noinc((SV *)hash);
}

int radio_enable_fft(int f) {
 _main_perform_fft = f != 0;
 return _main_perform_fft;
}

SV *radio_get_fft() {
 HV *hash;
 uint32_t i;
 uint32_t sr, fr;
 have_fft = dp_conc_q_peaks_try_pop_all(&_dsp_chain_peaks_queue,&pts);
 hash = newHV();
 hv_stores(hash,"have_fft",newSViv(have_fft));
 if (have_fft) {
  ___PERL_INSERT_HASH_COPYING_pts 
  AV *av;
  av = newAV();
  sr = _main_sample_rate;
  fr = _main_freq;
  /*
  dp_radio2832_dev_cmd(radio,GT_SR,&sr);
  dp_radio2832_dev_cmd(radio,GT_FREQ,&fr);
  */
  for (i=0;i<pts.actpts;i++) { 
   HV *h2;
   float f;
   h2 = newHV();
   hv_stores(h2,"index",newSViv(pts.points[i]->bin));
   hv_stores(h2,"dB",newSVnv(pts.points[i]->db));
   hv_stores(h2,"abs",newSVnv(pts.points[i]->abs));
   f = (float)pts.points[i]->bin / (float)pts.length;
   f *= (float)sr;
   f -= 0.5 * (float)sr;
   f += (float)fr;
   hv_stores(h2,"f",newSVnv(f));
   av_push(av,newRV_noinc((SV *)h2));
  }
  hv_stores(hash,"points",newRV_noinc((SV *)av));
 }
 return newRV_noinc((SV *)hash);
}


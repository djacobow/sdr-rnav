#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>


#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#endif

/* #include "ctrl_thread.h" */ /* temporary until get working */
/* #include <winsock2.h> */
#include "dp_radio2832.h"
#include "dsp_chain.h"
#include "ctrl_thread.h"
#include "my_console.h"
#include "main.h"

/* a few forward decs for functions later in this file. */

void getArgs(int argc, char *argv[],unsigned int *f, int *t, char *fn, float *m, dp_bool_t *um, dp_bool_t *pf, float *rc, float *snr);

/* These defaults will all correctly decode the OAK file listed below,
   and they can all be overriddent from the command line with getArgs() */
int             _main_max_cap_seconds  = 15; /* neg indicates forever */
unsigned int    _main_freq             = 116800000;
dp_bool_t       _main_use_mixer        = true;
dp_bool_t       _main_perform_fft      = false;
float           _main_mixer_lo_freq    = 14730;
float           _main_radial_calibrate = 197;
char            _main_infile[MAX_FN_LEN];
char            _main_odir[MAX_FN_LEN];
float           _main_min_snr          = 10.0;
float           _main_fft_pk_thresh    = 10.0;
volatile int    _main_decode_has_begun = 0;
volatile int    _main_ctrl_has_begun   = 0;

#ifdef __WIN32
const char *idir = "..\\test_input_files\\";
#else
const char *idir = "../test_input_files/";
#endif

#ifdef __linux
void handler(int sig) {
 void *array[10];
 size_t size;
 size = backtrace(array, 100);
 fprintf(stderr, "Error: signal %d\n", sig);
 backtrace_symbols_fd(array, size, STDERR_FILENO);
 exit(1);
}
#endif

int main(int argc, char*argv[]) {
 dp_base_t *radio;
 pthread_t dsp_thread;
#ifdef USE_RADIO
 pthread_t rtl_thread;
 int dev_count, index;
#endif

#ifdef _WIN32
/* This turns off the error dialog that Windows throws up. 
   Just a time saver when you're debugging a crashy program. */
 DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
 SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif

#ifdef _WIN32
 strncpy(_main_odir,"..\\test_output_files\\",MAX_FN_LEN);
#else
 strncpy(_main_odir,"../test_output_files/",MAX_FN_LEN);
#endif
 strcpy(_main_infile, idir);
 strcat(_main_infile, "OAK_r324_20130429_162637Z_116782kHz_250000sps_IQ.wav");

#ifdef __linux
 signal(SIGSEGV, handler);
 signal(SIGABRT, handler);
#endif

 getArgs(argc, argv, 
        &_main_freq, 
        &_main_max_cap_seconds, 
        _main_infile, 
        &_main_mixer_lo_freq,
        &_main_use_mixer,
        &_main_perform_fft,
        &_main_radial_calibrate,
        &_main_min_snr);

#ifdef USE_RADIO
 /* initialize and open the rtlsdr device.  */
 DP_CREATE(radio,radio2832);
 dp_set_name(radio,"radio");
 dev_count = dp_radio2832_list_devices(radio);
 index     = 0;
 if (index >= dev_count) {
  fprintf(stderr,"-err-- no such device index\n");
  exit(-1);
 }
 dp_radio2832_open_device(radio,index);
#endif

 /* split off the program into two threads, one for the 
  DSP and one for asynchronously pulling data from 
  the RTL device */

 pthread_create(&dsp_thread,NULL,dsp_thread_fn,radio);
#ifdef USE_RADIO
 pthread_create(&rtl_thread,NULL,rtl2832_thread_fn,radio);
#endif
 /* pthread_create(&ctrl_thread,NULL,ctrl_thread_fn,radio); */

 /* The line below just interlocks the control thread and 
    the decode thread so that the ctrl thread doesn't start
    until the decode thread has initialized and vice versa.
    Probably there are better ways to do this.
  */
 while (!_main_decode_has_begun) { /* my_sleep(10); */ };
 my_sleep(100);

#if 0
 pthread_create(&ctrl_thread,NULL,ctrl_thread_fn,radio);
 pthread_join(ctrl_thread, NULL);
#else
 ctrl_thread_fn(radio);
#endif

 pthread_join(dsp_thread, NULL);
#ifdef USE_RADIO
 /* pthread_join(rtl_thread,NULL); */
#endif

 /* pthread_join(ctrl_thread,NULL); */
 fprintf(stderr,"-info- all threads complete.\n");

#ifdef USE_RADIO
 /* should really dp_destroy(radio); */
 return(dp_radio2832_close_device(radio));
#else
 return(0);
#endif

}

void getArgs(int argc, char *argv[],unsigned int *f, int *t,
              char *fn, float *m, dp_bool_t *use_mixer,
              dp_bool_t *use_fft,
              float *radial_calibrate, float *min_snr) {
 int i;
 if (argc > 1) {
  for (i=0;i<argc;i++) {
   char *arg = argv[i];
   if (!strcmp(arg,"-f")) {
    arg = argv[++i];
    *f = atol(arg);
   } else if (!strcmp(arg,"-t")) {
    arg = argv[++i];
    *t = atol(arg);
   } else if (!strcmp(arg,"-i")) {
    arg = argv[++i];
    strcpy(fn,arg);
   } else if (!strcmp(arg,"-s")) {
    *use_fft = true;
   } else if (!strcmp(arg,"-m")) {
    arg = argv[++i];
    *m = atof(arg);
    if (*m == 0) {
     *use_mixer = false;
    } else {
     *use_mixer = true;
    } 
   } else if (!strcmp(arg,"-r")) {
    arg = argv[++i];
    *radial_calibrate = atof(arg);
   } else if (!strcmp(arg,"-c")) {
    arg = argv[++i];
    *min_snr = atof(arg);
   }
  }
 }
}


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

#include "ctrl_thread.h"
// #include <winsock2.h>
#include "rtl_help.h"
#include "dsp_chain.h"
#include "my_console.h"
#include "main.h"

// a few forward decs for functions later in this file.
std::string makeFileName(std::string base, uint32_t freq);
void getArgs(int argc, char *argv[],unsigned int *f, int *t, std::string &fn, float *m, bool *um, bool *pf, float *rc, float *snr, int *port);

// These defaults will all correctly decode the OAK file listed below,
// and they can all be overriddent from the command line with getArgs()
int             _main_max_cap_seconds  = 15; // neg indicates forever
unsigned int    _main_freq             = 116800000;
bool            _main_use_mixer        = true;
bool            _main_perform_fft      = false;
float           _main_mixer_lo_freq    = 15000;
float           _main_radial_calibrate = -175;
std::string     _main_infile           = idir + "OAK_r324_20130429_162637Z_116782kHz_250000sps_IQ.wav";
float           _main_min_snr          = 10.0;
int             _main_port             = 4044;

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

#ifdef _WIN32
// This turns off the error dialog that Windows throws up. 
// Just a time saver when you're debugging a crashy program.
DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif

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
	 &_main_min_snr,
	 &_main_port);

#ifdef USE_RADIO
 // initialize and open the rtlsdr device. 
 djdsp::rtl_help_c radio;
 radio.set_name("radio");
 int dev_count = radio.list_rtl_devices();
 int index     = 0;
 if (index >= dev_count) {
  std::cerr << "-err-- no such device index" << std::endl;;
  exit(-1);
 }
 radio.open_rtl_device(index);
#else
 // this is unused but some functions expect to be handed
 // "something" in the spot of the radio even when reading
 // from file, so this is that dummy.
 int radio;
#endif

 // split off the program into two threads, one for the 
 // DSP and one for asynchronously pulling data from 
 // the RTL device
 pthread_t dsp_thread;
 // pthread_t ctrl_thread;
#ifdef USE_RADIO
 pthread_t rtl_thread;
#endif

 pthread_create(&dsp_thread,NULL,dsp_thread_fn,&radio);
#ifdef USE_RADIO
 pthread_create(&rtl_thread,NULL,rtl_thread_fn,&radio);
#endif
 // pthread_create(&ctrl_thread,NULL,ctrl_thread_fn,&radio);

 // my original intent was to run the control stuff in its own 
 // thread, but the one extra thread proved too much for the 
 // RPi and it fell behind. So we will just run this function 
 // in the current context since it isn't doing anything anyway!
 my_sleep(100);
 ctrl_thread_fn(&radio);

 pthread_join(dsp_thread, NULL);
#ifdef USE_RADIO
 // pthread_join(rtl_thread,NULL);
#endif

 // pthread_join(ctrl_thread,NULL);
 std::cout << "-info- all threads complete" << std::endl;

#ifdef USE_RADIO
 int rv = radio.close_rtl_device();
#else
 int rv = 0;
#endif
 return rv;

};

std::string makeFileName(std::string base, uint32_t freq) {
 time_t now = time(0);
 tm *gmtm = gmtime(&now);
 int yr = gmtm->tm_year + 1900;
 int mo = gmtm->tm_mon  + 1;
 int dy = gmtm->tm_mday;
 
 int hr = gmtm->tm_hour;
 int mi = gmtm->tm_min;
 int sc = gmtm->tm_sec;
 
 char obuff[80];
 sprintf(obuff,"%s_%4.4d%2.2d%2.2d_%2.2d%2.2d%2.2dZ_%dkHz_IQ.wav",
   base.c_str(),yr,mo,dy,hr,mi,sc,freq/1000);
 std::string rstr = obuff;
 return rstr;
};

void getArgs(int argc, char *argv[],unsigned int *f, int *t,
		std::string &fn, float *m, bool *use_mixer,
		bool *use_fft,
		float *radial_calibrate, float *min_snr,
		int *port) {
 if (argc > 1) {
  for (int i=0;i<argc;i++) {
   char *arg = argv[i];
   if (!strcmp(arg,"-f")) {
    arg = argv[++i];
    *f = atol(arg);
   } else if (!strcmp(arg,"-t")) {
    arg = argv[++i];
    *t = atol(arg);
   } else if (!strcmp(arg,"-i")) {
    arg = argv[++i];
    fn = arg;
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
   } else if (!strcmp(arg,"-p")) {
    arg = argv[++i];
    *port = atol(arg);
   } else if (!strcmp(arg,"-s")) {
    arg = argv[++i];
    *min_snr = atof(arg);
   }
  };
 }
}


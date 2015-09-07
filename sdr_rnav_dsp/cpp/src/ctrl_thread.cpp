
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#include "ctrl_thread.h"
#include "receiver_stat.h"
#include <inttypes.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "main.h"
#include "rnav.h"
#include <assert.h>
#include "dsp_chain.h"
#include "conc_str_queue.h"
#include "conc_rstat_queue.h"
#include "conc_peaks_queue.h"
#ifdef USE_LCD
 #include "lcdproc.h"
#endif
#include "sserv.h"
#ifdef USE_SOCK_SERVER
 #include "radio_server.h"
#endif
#include "my_console.h"
#include "rtl_help.h"
#include "find_peaks.h"

// This is to turn off uninitialized warnings on the "lrstat" variables,
// which are set in parameter functions rather than on the left side of
// equal signs
#pragma GCC diagnostic ignored "-Wuninitialized"

/*

This function (ctrl_thread_fn) runs asynchronously from the radio+dsp
and essentially treats the radio as a separate, asynchronous "thing:" it
can change the radio's frequency and it can examine what's coming out of
the radio, that's it. The radio's output consists of two datastructures
passed to the outside world. 

One is receiver_stat_t a dumb struct that contains output results of
signal decoding.

The other is a std::string that contains the results of morse
decoding. These are passed out through two concurrency-friendly queues.
For the signal results queue, we enforce a maximum queue length; the
results of older buffers are just tossed if they haven't been looked
at. For the morse id queue, we are not enforcing a maximum length. It
is up to the consumer to consume often enough that it doesn't grow
ridiculously large.

There is actually one more variable, _dsp_chain_all_done, which the
receiver will set to true when it is shutting down. Other threads
can check it to see if they should shut down. It does not have mutex
protection. These three varaibles are at file scope in dsp_chain.cpp
and marked extern in dsp_chain.h

*/


// For tuning in real-time using the arrow keys, these are the 
// sizes of the increments
const int freqIncrLarge         = 10000;
const int freqIncrSmall         = 1000;
const unsigned int loIncr       = 1000;
const unsigned int loMin        = 2000;


std::string
fft_peaks_to_json(djdsp::peak_pts_t *pts) {
 std::stringstream oss;
 oss << "{ \"ok\": 1, "
     << "\"length\": " << pts->length << ", "
     << "\"average\": " << pts->average << ", "
     << "\"iteration\": " << pts->iteration << ", "
     << "\"points\": [";
 for (uint32_t i=0;i<pts->points.size();i++) {
  oss << "{ \"idx\": " << pts->points[i].bin << ", \"db\": " 
      << pts->points[i].db << "}"
      << ((i < (pts->points.size()-1)) ? ", " : "");
 }
 oss << "] }";
 return oss.str();
};


std::string
stats_to_json(receiver_stat_t *prs,std::string *idstr) {
 std::stringstream ss;
 ss.clear();

 ss << "{ "
    << " \"have_carrier\": " << (prs->have_carrier ? '1' : '0') << ", "
    << " \"snr\": "          << prs->strength_ratio << ", "
    << " \"snr_lpf\": "      << prs->strength_ratio_lpf << ", "
    << " \"id_instr\": \""   << *idstr << "\", "
    << " \"freq\": "         << prs->tune_freq << ", "
    << " \"use_mixer\": "    << ((prs->use_mixer) ? 1 : 0) << ", "
    << " \"run_fft\": "      << ((prs->run_fft) ? 1 : 0) << ", "
    << " \"mixer_lo\": "     << prs->mixer_lo_freq << ", "
    << " \"buffer_ct\": "    << prs->buffer_count<< ", ";

 if (prs->have_carrier) {
  float period        = prs->nf_sr / 30.0;
  float cyc_fract     = prs->phase_diff / period;
  float cyc_fract_lpf = prs->phase_diff_lpf / period;
  float angle     = (360.0 * cyc_fract     + _main_radial_calibrate);
  float angle_lpf = (360.0 * cyc_fract_lpf + _main_radial_calibrate);
  if (angle < 0)     { angle += 360.0; };
  if (angle_lpf < 0) { angle_lpf += 360.0; };
  ss << " \"ref30_period\": " << (float)prs->nf_sr*(1.0/prs->ref30_period) << ", "
     << " \"var30_period\": " << (float)prs->nf_sr*(1.0/prs->var30_period) << ", "
     << " \"radial\": "       << angle << ", "
     << " \"radial_lpf\": "   << angle_lpf << ", ";
 }
 ss << " \"ok\": 1 } " << std::endl;
 return ss.str();
}


void *ctrl_thread_fn(void *f) {

 std::cout << "-info- (ctrl) starting ctrl routine" << std::endl;
 uint32_t last_buffer    = 0;

#ifdef USE_SOCK_SERVER
 sock_serve_c sock_server;
 sock_server.init(_main_port,true);
 bool sock_needs_status = false;
 bool sock_needs_fft    = false;
#endif

#ifdef USE_LCD
 lcdproc_c lcd;
 lcd.connect(DEFAULT_LCD_HOST,DEFAULT_LCD_PORT);
#endif

 receiver_stat_t lrstat;
 djdsp::peak_pts_t      pts;
 std::string id_instr = "";
 while (!_dsp_chain_all_done) {
  bool have_status = _dsp_chain_rstat_queue.try_pop_all(lrstat);
  bool have_fft    = _dsp_chain_peaks_queue.try_pop_all(pts);
  if (have_fft) {
   std::cout << fft_peaks_to_json(&pts) << std::endl;
  }

#ifdef USE_SOCK_SERVER
  // d4(have_status, sock_needs_status, sock_needs_fft, have_fft);
  if (sock_needs_status && have_status) {
   std::string os = stats_to_json(&lrstat,&id_instr);
   sock_server.send_string(makeResp(os));
   id_instr.clear();
   sock_needs_status = false;
  } 
  if (sock_needs_fft && have_fft) {
   std::string os = fft_peaks_to_json(&pts);
   sock_server.send_string(makeResp(os));
   sock_needs_fft = false;
  }
#endif

  if (have_status) {
   std::string a_bit;
   while (_dsp_chain_id_text_queue.try_pop(a_bit)) {
    id_instr += a_bit;
   }
   int excess_string = id_instr.size() - 10;
   if (excess_string > 0) {
    id_instr.erase(0,excess_string);
   }

   if (lrstat.buffer_count != last_buffer) {
    std::cout << stats_to_json(&lrstat,&id_instr) << std::endl;

#ifdef USE_LCD
     std::stringstream oss1,oss2;
     std::string ident = findnletters(id_instr, 3);
     oss1 << "id : " << ((ident.size() == 3) ? ident : "___")
#ifdef USE_RADIO
          << " frq: " << std::setprecision(4) 
          << ((float)lrstat.radio->getCurrFreq()/1.0e6);
#else
          << " frq: " << "_na";
#endif
     float angle_lpf = (360.0 * lrstat.phase_diff_lpf / (lrstat.nf_sr / 30.0)) + _main_radial_calibrate;
     if (angle_lpf < 0) { angle_lpf += 360.0; };
     oss2 << "snr: " << std::setprecision(2) << (lrstat.strength_ratio)
          << "  rad: " << std::setprecision(4) << angle_lpf;
     lcd.print(0,0,oss1.str());
     lcd.print(1,0,oss2.str());
#endif

   }
  } 

#ifdef USE_RADIO
  if (my_kbhit()) _dsp_chain_all_done |= pKeys(&lrstat);
#ifdef USE_SOCK_SERVER
  bool still_connected = sock_server.client_connected();
  if (still_connected) {
   radio_server_cmd_t cmd = radio_server_check_commands(&sock_server);
   radio_server_execute_command(cmd,(djdsp::rtl_help_c *)f,&sock_server,&_dsp_chain_all_done,&sock_needs_status,&sock_needs_fft,&id_instr); 
  } else {
   std::cout << "client not connected\n";
   sock_server.answer_client();
  }
#endif
#endif

#ifdef USE_RADIO
  // my_sleep(1000);
  if (!have_status) {
   //my_sleep(lrstat.block_time_ms/4);
   my_sleep(100);
  } 
  // my_sleep(250);
#endif

  last_buffer = lrstat.buffer_count;
 }
 return 0;
};



void radio_server_execute_command(radio_server_cmd_t cmd,
		 djdsp::rtl_help_c *radio, 
		 sock_serve_c *ss,
		 bool *all_done,
		 bool *need_status,
		 bool *need_fft,
		 std::string *pid_instr) {
 switch (cmd.cmd) {
  case rs_invalid: {
   std::cout << "INVALID COMMAND\n";
   ss->send_string(makeResp("{ \"ok\": 0, \"msg\": \"invalid\"}"));
   break;
  }
  case rs_setmix: {
   std::cout << "MIXER COMMAND\n";
   unsigned int lo_freq = cmd.arg;
   if (lo_freq) {
    _main_use_mixer = true;
    _main_mixer_lo_freq = lo_freq;
   } else {
    _main_use_mixer = false;
   // this is a dummy value, but it should be something feasible 
   // so the mixer code doesn't complain
    _main_mixer_lo_freq = 15000; 
   }
   ss->send_string(makeResp("{\"ok\": 1, \"msg\": \"mixer updated\"}"));
   break;
  }
  case rs_setfft: {
   std::cout << "FFT ONLY\n";
   bool fft_on = cmd.arg;
   const uint32_t fft_if_sr = 1024000;
   const uint32_t dec_if_sr =  250000; /* this should be from dsp_chain: if_sr */
   std::string r =  "{\"ok\": 1, \"do_fft\": ";
   r.append(fft_on ? "1" : "0");
   // r.append("\"sample_rate\": ");
   // r.append(string(itoa(fft_if_sr));
   r.append(", \"msg\": \"fft_mode\"}");
   ss->send_string(makeResp(r));
   _main_perform_fft = fft_on;
   uint32_t real_sr = radio->dev_cmd(djdsp::ST_SR,fft_on ? fft_if_sr : dec_if_sr);
   std::cout << "sample rate return: " << real_sr << std::endl;
   *need_status = false;
   break;
  }
  case rs_showfft: {
   std::cout << "SHOW FFT\n";
   *need_fft = true;
   break;
  }
  case rs_tune: {
   std::cout << "TUNE COMMAND\n";
   radio->dev_cmd(djdsp::ST_FREQ,cmd.arg);
   _main_freq = cmd.arg;
   pid_instr->clear();
   ss->send_string(makeResp("{\"ok\": 1, \"msg\": \"retuned\"}"));
   break;
  }
  case rs_quit: {
   ss->send_string(makeResp("{\"ok\": 1, \"msg\": \"dropping connection\" }"));
   ss->disconnect_client();
   std::cout << "QUIT COMMAND\n";
   break;
  }
  case rs_shutdown: {
   ss->send_string(makeResp("{\"ok\": 1, \"msg\": \"shutting down\" }"));
   *all_done |= 1;
   std::cout << "SHUTDOWN COMMAND\n";
  }
  case rs_status: {
   *need_status = true;
   std::cout << "STATUS COMMAND\n";
   break;
  }
  case rs_clearid: {
   ss->send_string(makeResp("{\"ok\": 1, \"msg\": \"id_string cleared\" }"));
   std::cout << "CLEAR ID INSTR\n";
   pid_instr->clear();
  }
  default: {
  }
 }
}

bool pKeys(receiver_stat_t *lrs) {
 bool d = false;
 static std::string fstr = "";
 command_keys_t k = get_cmd_key();
 std::cout << "-info- (ctrl) command: " << k.cmd << " letter " << k.c << std::endl;
 switch (k.cmd) {
  case ck_q:
   d = true;
   break;
  case ck_digit:
   fstr += k.c;
   break;
  case ck_enter: {
   uint32_t nfreq = atol(fstr.c_str());
   lrs->radio->dev_cmd(djdsp::ST_FREQ,nfreq);
   _main_freq = nfreq;
   std::cout << "-info- (ctrl) NEW FREQ: " << nfreq << std::endl;
   fstr.clear();
   break;
  }
  case ck_uparrow: {
   uint32_t cfreq = lrs->radio->getCurrFreq();
   cfreq += freqIncrLarge;
   lrs->radio->dev_cmd(djdsp::ST_FREQ,cfreq);
   _main_freq = cfreq;
   std::cout << "-info- (ctrl) NEW FREQ: " << cfreq << std::endl;
   break;
  }
  case ck_downarrow: {
   uint32_t cfreq = lrs->radio->getCurrFreq();
   cfreq -= freqIncrLarge;
   lrs->radio->dev_cmd(djdsp::ST_FREQ,cfreq);
   _main_freq = cfreq;
   std::cout << "-info- (ctrl) NEW FREQ: " << cfreq << std::endl;
   break;
  }
  case ck_leftarrow: {
   uint32_t cfreq = lrs->radio->getCurrFreq();
   cfreq -= freqIncrSmall;
   lrs->radio->dev_cmd(djdsp::ST_FREQ,cfreq);
   _main_freq = cfreq;
   std::cout << "-info- (ctrl) NEW FREQ: " << cfreq << std::endl;
   break;
  }
  case ck_rightarrow: {
   uint32_t cfreq = lrs->radio->getCurrFreq();
   cfreq += freqIncrSmall;
   lrs->radio->dev_cmd(djdsp::ST_FREQ,cfreq);
   _main_freq = cfreq;
   std::cout << "-info- (ctrl) NEW FREQ: " << cfreq << std::endl;
   break;
  }
  case ck_s: {
   _main_perform_fft = !_main_perform_fft;
   std::cout << "-info- (ctrl) turning " 
	     << ((_main_perform_fft) ? "on" : "off") 
	     << " fft" << std::endl;
   break;
  }
  case ck_m: {
   _main_use_mixer = ~_main_use_mixer;
   std::cout << "-info- (ctrl) turning " 
	     << (_main_use_mixer ? "on" : "off") 
	     << " mixer" << std::endl;
   break;
  }
  case ck_z: {
   _main_mixer_lo_freq -= loIncr;
   if (_main_mixer_lo_freq < loMin) { _main_mixer_lo_freq = loMin; };
   bool lo_impossible = lrs->mixer->set_lo_freq(_main_mixer_lo_freq);
   if (!lo_impossible) {
    std::cout << "-info- (ctrl) adjusting lo to: " << _main_mixer_lo_freq << std::endl;
   } else {
    std::cout << "-warn- (ctrl) LO freq too low. Turning off mixer." << std::endl;
    _main_use_mixer = false;
   }
   break;
  }
  case ck_x: {
   _main_mixer_lo_freq += loIncr;
   bool lo_impossible = lrs->mixer->set_lo_freq(_main_mixer_lo_freq);
   if (!lo_impossible) {
    std::cout << "-info- (ctrl) adjusting lo to: " << _main_mixer_lo_freq << std::endl;
   } else {
    std::cout << "-warn- (ctrl) LO freq too low. Turning off mixer." << std::endl;
    _main_use_mixer = false;
   }
   break;
  }
  default: {
  }
 }
 return d;
};


std::string uint2string(unsigned int i, unsigned int w) {
 std::stringstream ss;
 ss.fill('0');
 ss.width(w);
 ss << i;
 return ss.str();
}

std::string makeResp(std::string is) {
 unsigned int l = is.size();
 std::string ls = uint2string(l,3);
 return ls + is;
};

// simple routine to find n consecutive letters in a string and
// return the first such occurrence in the string. This should
// weed out a lot of morse garbage and return a station ID most
// of the time.
std::string findnletters(const std::string is, const uint32_t n) {
 std::string os;
 os.resize(n);
 uint32_t ct = 0;
 for (uint32_t i=0;i<is.length();i++) {
  char inch = is[i];
  if ((inch >= 'A') && (inch <= 'Z')) {
   os[ct] = inch;
   ct++; 
  } else {
   ct = 0;
  }
  if (ct == n) {
   return os;
  }
 }
 return std::string("");
};



// With the socket interface we'll do these from a high level or scripting language.
// ... tired of c++

/*

void cold_scan_for_stations(uint32_t sfreq, uint32_t efreq, uint32_t fstep, djdsp::rtl_help_c *radio, vors_stat_c *vors) {
 for (uint32_t freq=sfreq;freq<=efreq;freq+=fstep) {
  radio->dev_cmd(djdsp::ST_FREQ,freq);
  bool freq_tested = false;
  receiver_stat_t lrstat;
  uint32_t starting_bcount = lrstat.buffer_count;
  while (_dsp_chain_rstat_queue.try_pop(lrstat)) { };
  while (!freq_tested) {
   my_sleep(250);
   while (_dsp_chain_rstat_queue.try_pop(lrstat)) { };
   if ((lrstat.buffer_count - starting_bcount) > MIN_DWELL_BLOCKS) {
    if (lrstat.strength_ratio > 30) {
     float period    = lrstat.nf_sr / 30.0;
     float cyc_fract = lrstat.phase_diff / period;
     float angle     = (int)(360.0*cyc_fract) % 360;
     vors->update_vor(0,freq,angle,lrstat.strength_ratio);
    }    
    freq_tested = true;
   }
  }
 }
}

void long_scan_for_ids(djdsp::rtl_help_c *radio, vors_stat_c *vors) {
 flist_t freqs = vors->getFreqs();
 for (uint32_t i=0;i<freqs.size();i++) {
  uint32_t freq_khz = freqs[i];
  radio->dev_cmd(djdsp::ST_FREQ,freq_khz*1000);
  my_sleep(ID_DWELL_TIME * 1000);
  receiver_stat_t lrstat;
  while (_dsp_chain_rstat_queue.try_pop(lrstat)) { };
  std::string a_bit;
  std::string id_instr;
  while (_dsp_chain_id_text_queue.try_pop(a_bit)) {
   id_instr += a_bit;
  }
  std::string ident = findnletters(id_instr, 3);
  if (ident.length() == 3) {
   float period    = lrstat.nf_sr / 30.0;
   float cyc_fract = lrstat.phase_diff / period;
   float angle     = (int)(360.0*cyc_fract) % 360;
   vors->update_vor(ident.c_str(),freq_khz,angle,lrstat.strength_ratio);
  }
 }
};

*/






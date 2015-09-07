
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz

#include "sserv.h"
#include "radio_server.h"

#include <string>
#include <iostream>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

// trim from start
static inline std::string &ltrim(std::string &s) {
 s.erase(s.begin(), 
  std::find_if(s.begin(), 
        s.end(), 
        std::not1(std::ptr_fun<int, int>(std::isspace))
       )
  );
 return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
 s.erase(std::find_if(s.rbegin(), 
               s.rend(), 
        std::not1(std::ptr_fun<int, int>(std::isspace))).base(), 
                  s.end());
 return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
 return ltrim(rtrim(s));
}


radio_server_cmd_t
radio_server_check_commands(sock_serve_c *pss) {
 radio_server_cmd_t rv;
 rv.cmd = rs_nothing;
 rv.arg = 0;

 if (pss->client_connected()) {
  std::string count_str;
  int chars_read = pss->receive_string(count_str,3);
  // std::cout << "chars read " << chars_read << std::endl;
  if (chars_read == 3) {
   int bytes_to_read = atol(count_str.c_str());
   // std::cout << "count string: " << count_str;
   std::string incmd;
   incmd = "";
   chars_read = pss->receive_string(incmd, bytes_to_read);
   if (chars_read) {
    std::string cmd = trim(incmd);
    if (cmd.size()) {
     if        (!cmd.compare("quit")) {
      rv.cmd = rs_quit;
     } else if (!cmd.compare("shutdown")) {
      rv.cmd = rs_shutdown;
     } else if (!cmd.compare("status")) {
      rv.cmd = rs_status;
     } else if (!cmd.compare("clearid")) {
      rv.cmd = rs_clearid;
     } else if ((cmd.size() > 6) && (!cmd.substr(0,6).compare("setfft"))) {
      std::string nstr = cmd.substr(6,cmd.size()-6);
      nstr = trim(nstr);
      std::cout << "change fft mode string: \"" << nstr << "\"" << std::endl;
      unsigned int f_val = atol(nstr.c_str());
      rv.cmd = rs_setfft;
      rv.arg = f_val;
     } else if (!cmd.compare("showfft")) {
      rv.cmd = rs_showfft;
     } else if ((cmd.size() > 5) && (!cmd.substr(0,4).compare("tune"))) {
      std::string nstr = cmd.substr(4,cmd.size()-4);
      nstr = trim(nstr);
      unsigned int f_val = atol(nstr.c_str());
      rv.cmd = rs_tune;
      rv.arg = f_val;
     } else if ((cmd.size() > 9) && (!cmd.substr(0,8).compare("mixer_lo"))) {
      std::string nstr = cmd.substr(8,cmd.size()-8);
      nstr = trim(nstr);
      unsigned int f_val = atol(nstr.c_str());
      rv.cmd = rs_setmix;
      rv.arg = f_val;
     } else {
      rv.cmd = rs_invalid;
     }
    }   
   } else {
   }
  }
 } else {
  std::cout << "-warn- server client disconnected?" << std::endl;
  rv.cmd = rs_quit;
 }
 return rv;
};


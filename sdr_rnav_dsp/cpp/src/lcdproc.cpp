
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#include "lcdproc.h"
#include <iostream>
#include <sstream>
#include <stdlib.h>


/*
 
 Simple wrapper class to talk to an LCD display on the Raspberry Pi.

 There are a lot of brittle assumptions baked into this code, so listen 
 close.

 The LCD display I have is a 20x4 line HD44780 device driven by a 
 MCP23008 i2c decoder chip, which is in turn driven by the i2c pins
 on the RPi's GPIO header.

 The actual side is handled by a Linux combination of daemons called
 LCDd and LCDproc. The former interfaces with the kernel mode 
 i2c functions and gets all its commands from LCDproc. LCDproc
 host a socket port and takes commands which it forward to LCDd and
 ultimately to the LCD display.

 Note also that driver package that comes with LCDd/LCDproc 
 contains an i2c driver that expects to talk to another chip. I 
 used a patch I found on the internet, here: 

 http://forums.adafruit.com/viewtopic.php?f=19&t=35581

 to make a version that talks to the MCP23008 chip which is the one
 that is used in the Adafruit LCD backback.

 It's actually a ridiculous amount of complexity to drive an LCD, but
 it is flexible. One could theoretically change the LCD itself as 
 well as how its connected, and you'd only have to chnage config 
 files on the computer. The code below should still work.

 Well, we'll see....

 -- dave j Jul 2013
*/

#ifdef __linux


lcdproc_c::lcdproc_c() {
 width = DEFAULT_WIDTH;
 height = DEFAULT_HEIGHT;
}

lcdproc_c::~lcdproc_c() {

};

int
lcdproc_c::connect(const std::string host, const int port) {
 s.create();
 s.connect(host,port);
 bool connected = false;
 int ok = s.is_valid();
 if (ok) {
  // s.set_non_blocking(true);
  sout("hello\n");
  std::string tmp;
  int ok = s.recv(tmp);

  std::stringstream iss;
  iss.str(tmp);
  while (!iss.eof()) {
   std::string chunk;
   iss >> chunk;
   if (!chunk.compare("wid")) {
    iss >> chunk;
    width = atol(chunk.c_str());
   } else if (!chunk.compare("hgt")) {
    iss >> chunk;
    height = atol(chunk.c_str());
   } else if (!chunk.compare("LCDproc")) {
    connected |= true;
   }
  }
//connect LCDproc 0.5.6 protocol 0.3 lcd wid 20 hgt 4 cellwid 5 cellhgt 8

  if (connected) {
   s.set_non_blocking(true);
   sout("client_set name {sdr_rnav}\n");
   sout("screen_add s1\n");
   sout("screen_set s1 name {sdr_rnav_s1}\n");
   sout("screen_set s1 heartbeat off\n");
   // sout("widget_add s1 title title\n");
   // sout("widget_set s1 title {sdr_rnav}\n");
   for (int i=0;i< height; i++) {
    std::stringstream ss;
    ss << "widget_add s1 l" << i << " string\n";
    sout(ss.str());
   } 
  } else {
  std::cout << "-err- (lcdproc) socket open but no lcdproc answered"
            << std::endl;
  return -1;
  }
 } else {
  std::cout << "-err- (lcdproc) could not open socket " 
            << host << " " << port << std::endl;
  return -2;
 }
 return 0;
};

int lcdproc_c::print(unsigned int row, unsigned int col, std::string is) {
 std::stringstream ss;

 int len = width - col;
 std::string os = is.substr(0,len);

 ss << "widget_set s1 l" << row << " " << (col+1) << " " 
    << (row+1) << " {" << os << "}\n";

 sout(ss.str());
 return len;
};

void
lcdproc_c::sout(const std::string is) {
 // std::cout << "-info- (lcdproc) sendind: " << is << std::endl;
 if (s.is_valid()) {
  s.send(is);
 }
};

#endif


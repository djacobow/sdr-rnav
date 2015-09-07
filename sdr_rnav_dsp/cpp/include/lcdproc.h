
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

// Simple routines for talking to the LCDd daemon available
// on some linux machines, including the RPi if an LCD has 
// been attached.

#ifndef __LCDPROC_H
#define __LCDPROC_H

#ifdef __linux

#include "sclient.h"

#define DEFAULT_LCD_HOST "127.0.0.1"
#define DEFAULT_LCD_PORT (13666)
#define DEFAULT_WIDTH (16)
#define DEFAULT_HEIGHT (2)

class lcdproc_c {

 public:
  lcdproc_c();
  ~lcdproc_c();  
  int connect(const std::string host, const int port);
  int print(unsigned int row, unsigned int col, std::string s);
 private:
  void sout(const std::string is);
  sock_client_c s;
  int width;
  int height;
};

#endif

#endif


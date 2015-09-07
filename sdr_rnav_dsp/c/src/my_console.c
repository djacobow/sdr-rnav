
/* Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

//
// What you see below are some very basic and primitive console
// routines for controlling the executable from the keyboard while
// it's running.
//
// Here, we resurrect the ancient dos kbhit() and getch() routines
// and implement versions for Linux as well using termios and 
// ioctl magic that I don't really understand.
//
// -- dave j
*/


#include "my_console.h"

#ifdef __linux

/**
 Linux (POSIX) implementation of _kbhit().
 Morgan McGuire, morgan@cs.brown.edu
 */
#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
#include <stropts.h>
#include <asm-generic/ioctls.h>
#include <unistd.h>

void my_sleep(int ims) {
 usleep(ims*1000);
}

int my_kbhit() {
    struct termios term;
    static const int STDIN = 0;
    static int initialized = 0;
    int bytesWaiting;

    if (! initialized) {
        /* Use termios to turn off line buffering */
        /* termios term; */
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = 1;
    }

    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

static struct termios termios_old, termios_new;

/* Initialize new terminal i/o settings */
void initTermios(int echo) {
  tcgetattr(0, &termios_old); /* grab old terminal i/o settings */
  termios_new = termios_old; /* make new settings same as old settings */
  termios_new.c_lflag &= ~ICANON; /* disable buffered i/o */
  termios_new.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
  tcsetattr(0, TCSANOW, &termios_new); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) {
  tcsetattr(0, TCSANOW, &termios_old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) {
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char my_getch(void) {
  return getch_(0);
}

command_keys_t get_cmd_key() {
 char k = my_getch();
 command_keys_t rv;
 rv.cmd = ck_none; 
 rv.c   = k;
 switch (k) {
  case 'q' : rv.cmd = ck_q; break;
  case 'm' : rv.cmd = ck_m; break;
  case 'z' : rv.cmd = ck_z; break;
  case 'x' : rv.cmd = ck_x; break;
  case 10  : rv.cmd = ck_enter; break;
  case '0' :
  case '1' :
  case '2' :
  case '3' :
  case '4' :
  case '6' :
  case '7' :
  case '8' :
  case '9' : rv.cmd = ck_digit;
  case 27 :
  if (my_kbhit()) {
   k = my_getch();
   if (k == 91) {
    if (my_kbhit()) {
     k = my_getch();
     rv.c = k;
     switch (k) {
      case 65 : rv.cmd = ck_uparrow; break;
      case 66 : rv.cmd = ck_downarrow; break;
      case 68 : rv.cmd = ck_leftarrow; break;
      case 67 : rv.cmd = ck_rightarrow; break;
     }
    }
   }
  }
  break;
 }
 return rv;
}

#endif

#ifdef __WIN32

#include <conio.h>
#include <windows.h>

void my_sleep(int ims) {
 Sleep(ims);
}

#include <stdio.h>

/* included to work around bugs in Perl Inline:CPP. These are not used and 
// sill never be called.
*/
void initTermios(int echo) { }
char getch_(int echo) { return 0;}


int my_kbhit() {
 return kbhit();
}

char my_getch() {
 return getch();
}

command_keys_t get_cmd_key() {
 unsigned char k = (unsigned char)my_getch();
 command_keys_t rv;
 rv.cmd = ck_none; 
 rv.c   = k;
 switch (k) {
  case 'q' : rv.cmd = ck_q; break;
  case 'm' : rv.cmd = ck_m; break;
  case 'z' : rv.cmd = ck_z; break;
  case 'x' : rv.cmd = ck_x; break;
  case 's' : rv.cmd = ck_s; break;
  case 13  : rv.cmd = ck_enter; break;
  case '0' :
  case '1' :
  case '2' :
  case '3' :
  case '4' :
  case '6' :
  case '7' :
  case '8' :
  case '9' : rv.cmd = ck_digit;
  case 224 :
  if (my_kbhit()) {
   k = my_getch();
   rv.c = k;
   switch (k) {
    case 72 : rv.cmd = ck_uparrow; break;
    case 80 : rv.cmd = ck_downarrow; break;
    case 75 : rv.cmd = ck_leftarrow; break;
    case 77 : rv.cmd = ck_rightarrow; break;
   }
  }
  break;
 }
 return rv;
}

#endif


// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef __MY_CONSOLE_H
#define __MY_CONSOLE_H

// minimalist console routines for controlling the executable
// in real-time

int my_kbhit();
char my_getch();
void my_sleep(int);

typedef enum {
 ck_unknown,
 ck_none,
 ck_q,
 ck_m,
 ck_z,
 ck_x,
 ck_s,
 ck_digit,
 ck_enter,
 ck_leftarrow,
 ck_rightarrow,
 ck_uparrow,
 ck_downarrow,
} command_keys_enum_t;

typedef struct command_keys_t {
 command_keys_enum_t cmd;
 char c;
} command_keys_t;

command_keys_t get_cmd_key();

#endif


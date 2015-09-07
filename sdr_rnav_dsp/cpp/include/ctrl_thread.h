
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz

#ifndef __CTRL_THREAD_H
#define __CTRL_THREAD_H

#include <string>
#include "receiver_stat.h"
#include "radio_server.h"
#include "sserv.h"

// minimum number of buffers to hang on one frequency
// (to let receiver settle) to determine if station is 
// there
#define MIN_DWELL_BLOCKS (4) 
// minimum time to sit on a frequency to be sure to pick
// up a complete ID
#define ID_DWELL_TIME (35)

void *ctrl_thread_fn(void *f);
bool pKeys(receiver_stat_t *lrs);
std::string stats_to_json(receiver_stat_t *prs, std::string *idstr);
void radio_server_execute_command(radio_server_cmd_t cmd,
		                  djdsp::rtl_help_c *radio, 
				  sock_serve_c *ss,
		                  bool *all_done,
		                  bool *need_status,
		                  bool *need_peaks,
				  std::string *pid_instr);

std::string uint2string(unsigned int i, unsigned int w);
std::string makeResp(std::string is);
std::string findnletters(const std::string, const uint32_t);
#endif


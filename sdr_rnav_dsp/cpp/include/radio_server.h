
#ifndef _radio_server_h
#define _radio_server_h

#include "sserv.h"

typedef enum radio_server_cmd_type_t {
 rs_quit,
 rs_shutdown,
 rs_tune,
 rs_setmix,
 rs_setfft,
 rs_status,
 rs_clearid,
 rs_showfft,
 rs_nothing,
 rs_invalid,
} radio_server_cmd_type_t;

typedef struct radio_server_cmd_t {
 radio_server_cmd_type_t cmd;
 long int arg;
} radio_server_cmd_t;

radio_server_cmd_t 
radio_server_check_commands(sock_serve_c *pss);


#endif


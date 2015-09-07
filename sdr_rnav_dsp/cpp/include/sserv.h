#ifndef _sock_serv_h
#define _sock_serv_h

#ifdef __linux

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#endif

#ifdef _WIN32

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL (0)
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

// This doesn't work for GCC anyway and generates a warning.
// #pragma comment(lib,"ws2_32.lib")

#endif

#include <string>
#include <stdio.h>


const int SSRV_MAXHOSTNAME = 200;
const int SSRV_MAXCONNECTIONS = 1;
const int SSRV_MAXRECV = 500;

class sock_serve_c {

 public:
  sock_serve_c();
  ~sock_serve_c();
  bool init(const int port, bool non_blocking);
  bool answer_client();

  bool is_listening();
  bool is_valid();
  bool client_connected();
  bool send_string(const std::string);
  int receive_string(std::string &);
  int receive_string(std::string &, int l);
  void disconnect_client();

 private:
  int m_sock;
  int c_sock;
  sockaddr_in m_addr;
#ifdef _WIN32
  int win_init_result;
#endif
  bool is_nonblock;
  bool client_error;
};


void set_non_blocking(int s, const bool b);
int  sock_bytes_available(int s);


#endif


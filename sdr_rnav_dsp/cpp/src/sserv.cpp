
// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Summer 2013
//
// Copyright 2013, David Jacobowitz

#include "sserv.h"
#include <iostream>
#include <string>
#include <errno.h>
#include <fcntl.h>

#ifdef __linux
#include <string.h>
#endif

#ifdef _WIN32
// #include "win_inet_pton.h"
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT  WSAEAFNOSUPPORT
#endif
#endif

sock_serve_c::sock_serve_c() {
 m_sock = -1;
 c_sock = -1;
 client_error = false;
 memset(&m_addr,0,sizeof(m_addr));
 is_nonblock = false;
#ifdef _WIN32
 WSADATA wsaData;
 win_init_result = WSAStartup(MAKEWORD(2,2), &wsaData);
 if (win_init_result != 0) {
  std::cerr << "WSAstartup failed: " << win_init_result << std::endl;
 }
#endif
};

sock_serve_c::~sock_serve_c() {
 if (client_connected()) {
  disconnect_client();
 }
 if (is_valid()) {
  close(m_sock); 
 }
};

bool
sock_serve_c::is_valid() {
 return (m_sock > 0);
}

bool
sock_serve_c::client_connected() {
 if (c_sock < 0) { 
  return false;
 } 
 return !client_error;
};

bool sock_serve_c::init(const int port, bool non_blocking) {
 m_sock = socket(AF_INET,SOCK_STREAM,0);
 if (m_sock < 0) {
  return false;
 }

 int on = 1;
 if (setsockopt(m_sock,SOL_SOCKET,SO_REUSEADDR, (const char *)&on, sizeof(on)) == -1) {
  return false;
 }

 m_addr.sin_family = AF_INET;
 m_addr.sin_addr.s_addr = INADDR_ANY;
 m_addr.sin_port = htons(port);
 int bind_return = bind(m_sock,(struct sockaddr *)&m_addr,sizeof(m_addr));
 if (bind_return < 0) {
  return false;
 }

 set_non_blocking(m_sock,non_blocking);
 is_nonblock = non_blocking;

 int listen_return = listen(m_sock,SSRV_MAXCONNECTIONS);
 if (listen_return < 0) {
  return false;
 }
 return true;
};


void
sock_serve_c::disconnect_client() {
 if (client_connected()) {
#ifdef _WIN32
   shutdown(c_sock,SD_BOTH);
#endif
#ifdef __linux
   shutdown(c_sock,SHUT_RDWR);
#endif
   c_sock = -1;
 }
};

bool
sock_serve_c::answer_client() {
 int addr_length = sizeof(m_addr);
 int avail = sock_bytes_available(m_sock);
 if (avail) {
  c_sock = accept(m_sock,(sockaddr *)&m_addr, (socklen_t *)&addr_length);
  if (c_sock > 0) { 
   client_error = 0;
   return true;
  }
 }
 return false;
};

bool
sock_serve_c::send_string(const std::string s) {
 if (client_connected()) {
  int status = send(c_sock,s.c_str(),s.size(), MSG_NOSIGNAL);
  if (status > 0) {
   std::cout << "-info- (sock_serve) sent " << status << " bytes: " << s << std::endl;
   return true;
  } else {
   client_error = true;
  }
 }
 return false;
};



int 
sock_bytes_available(int s) {
 fd_set readfds;
 struct timeval tv = {1, 0};
 FD_ZERO(&readfds);
 FD_SET(s, &readfds);
 tv.tv_sec = 1;
 tv.tv_usec = 0;
 int r = 0;
 r = select(s+1, &readfds, NULL, NULL, &tv);
 return r;
};


int
sock_serve_c::receive_string(std::string &s, int l) {
 int read = 0;
 if (client_connected()) {
  int bytes_avail = sock_bytes_available(c_sock);
  if (bytes_avail) {
   int to_read = l;
   s = "";
   while (read < l) {
    char buf[SSRV_MAXRECV+1]; memset(buf,0,SSRV_MAXRECV+1);
    int read_this_time = recv(c_sock,buf,to_read,0);
    if (read_this_time > 0) {
     to_read -= read_this_time;
     read += read_this_time;
     s.append(buf);
    }
   }
   std::cout << "-info- (sock_serve) received " << l << " bytes out of available " << bytes_avail << ": " 
            << s << std::endl;;
  } 
 }
 return read;
};


int 
sock_serve_c::receive_string(std::string &s) {
 if (client_connected()) {
  int bytes_avail = sock_bytes_available(c_sock);
  if (bytes_avail) {
   char buf[SSRV_MAXRECV+1];
   s = "";
   memset(buf,0,SSRV_MAXRECV+1);
   int status = recv(c_sock,buf,SSRV_MAXRECV,0);
   if (status == -1) {
    client_error = true;
   } else {
    s = buf;
   return status;
   }
  };
 }
 return 0;
};

/*
int
sock_serve_c::receive_string(std::string &s) {
 char buf[SSRV_MAXRECV+1];
 s = "";
 memset(buf,0,SSRV_MAXRECV+1);
 int status = recv(c_sock,buf,MAXRECV,0);
 // std::cout << "-debug- status " << status << " errno = " << errno << " in recv\n";
 if (status == 0) {
  client_error = true;
  return 0;
 } else if (status < 0) {
#ifdef __linux
  if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
#endif
#ifdef _WIN32
  if ((errno == ERANGE) || (errno == WSAEWOULDBLOCK)) {
#endif
   return 0;
  } else {
   client_error = true;
   std::cout << "status < 0 errno = " << errno << " in recv\n";
   return 0;
  }
 };
 s = buf;
 return status;
};
*/

void set_non_blocking(int s, const bool b) {
#ifdef __linux
 int opts;
 opts = fcntl(s,F_GETFL);
 if (opts<0) {
  return;
 };
 if (b) {
  opts = (opts | O_NONBLOCK);
 } else {
  opts = (opts & ~O_NONBLOCK);
 }
 fcntl(s,F_SETFL,opts);
#endif
#ifdef _WIN32
 unsigned long arg = b;
 ioctlsocket(s,FIONBIO, &arg);
#endif
};


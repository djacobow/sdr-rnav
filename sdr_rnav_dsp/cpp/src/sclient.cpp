// Author: David Jacobowitz
//         david.jacobowitz@gmail.com
//
// Date  : Spring 2013
//
// Copyright 2013, David Jacobowitz



#ifdef __linux

#include <iostream>
#include "sclient.h"
#include "string.h"
#include <string.h>
#include <errno.h>
#include <fcntl.h>


sock_client_c::sock_client_c() : m_sock ( -1 ) {
 memset ( &m_addr, 0, sizeof ( m_addr ) );
}

sock_client_c::~sock_client_c() {
 if ( is_valid() )
  ::close ( m_sock );
}

bool sock_client_c::send ( const std::string s ) const {
 int status = ::send ( m_sock, s.c_str(), s.size(), MSG_NOSIGNAL );
 if ( status == -1 ) {
  return false;
 } else {
  return true;
 }
}


bool sock_client_c::create() {
 m_sock = socket(AF_INET,SOCK_STREAM,0);
if (!is_valid()) {
 return false;
}
 int on = 1;
 if (setsockopt(m_sock,SOL_SOCKET,SO_REUSEADDR,(const char *)&on, sizeof(on)) == -1 )
  return false;
 return true;
};


int sock_client_c::recv ( std::string& s ) const {
 char buf [ SCL_MAXRECV + 1 ];

 s = "";

 memset ( buf, 0, SCL_MAXRECV + 1 );

 int status = ::recv ( m_sock, buf, SCL_MAXRECV, 0 );

 if ( status == -1 ) {
  std::cout << "status == -1   errno == " << errno << "  in sock_client_c::recv\n";
  return 0;
 } else if ( status == 0 ) {
  return 0;
 } else {
  s = buf;
  return status;
  }
}

bool sock_client_c::connect ( const std::string host, const int port ) {

 m_addr.sin_family = AF_INET;
 m_addr.sin_port = htons ( port );

 int status = inet_pton ( AF_INET, host.c_str(), &m_addr.sin_addr );

 if ( errno == EAFNOSUPPORT ) return false;

 status = ::connect ( m_sock, ( sockaddr * ) &m_addr, sizeof ( m_addr ) );

 if ( status == 0 ) {
  return true;
 } else {
  return false;
 }
}

void sock_client_c::set_non_blocking ( const bool b ) {

 int opts;

 opts = fcntl ( m_sock, F_GETFL );

 if ( opts < 0 ) {
  return;
 }

 if ( b )
  opts = ( opts | O_NONBLOCK );
 else
  opts = ( opts & ~O_NONBLOCK );

 fcntl ( m_sock, F_SETFL,opts );
}

#endif



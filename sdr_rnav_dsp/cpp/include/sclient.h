#ifndef sock_client_c_class
#define sock_client_c_class

#ifdef __linux

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>


// const int SCL_MAXHOSTNAME = 200;
// const int SCL_MAXCONNECTIONS = 5;
const int SCL_MAXRECV = 500;

class sock_client_c
{
 public:
  sock_client_c();
  virtual ~sock_client_c();

  // Client initialization
  bool create();
  bool connect ( const std::string host, const int port );

  // Data Transimission
  bool send ( const std::string ) const;
  int recv ( std::string& ) const;

  void set_non_blocking ( const bool );

  bool is_valid() const { return m_sock != -1; }

 private:

  int m_sock;
  sockaddr_in m_addr;

};


#endif

#endif

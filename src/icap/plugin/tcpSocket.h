#ifndef __TCP_SOCKET_H__
#define __TCP_SOCKET_H__

#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctime>
#include <exception>

#include "socket.h"
#include "ipAddr.h"

namespace utils
{

class TcpSocket : public Socket
{
private:
  IpAddr peer_;
  friend class ServerSocket;

public:
  TcpSocket() throw (std::ios_base::failure);
  TcpSocket(int fd);

  // return -errno
  int connect(const IpAddr &ip);

  ssize_t read(void *buf, size_t len);
  ssize_t write(const void *buf, size_t len);

  int readAll(void *buf, size_t len);
  int writeAll(const void *buf, size_t len);

  IpAddr getPeer();
  IpAddr getAddr() const;

  void valid() throw (std::logic_error);

  friend std::ostream &operator<<(std::ostream &os, const TcpSocket &sock);
};

std::ostream &operator<<(std::ostream &os, const TcpSocket &sock);

} // namespace utils

#endif // __TCP_SOCKET_H__

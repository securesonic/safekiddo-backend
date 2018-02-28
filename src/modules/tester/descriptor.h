#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <exception>

namespace utils {

class Descriptor
{
private:
  Descriptor(const Descriptor&);
  Descriptor& operator=(const Descriptor&);

protected:
  int fd_;

public:
  Descriptor() : fd_(-1) {}
  Descriptor(int fd) : fd_(fd) {}

public:
  void close();

  int setNonblocking();

  int getFd() const { return this->fd_; }

  int waitRead(int tout);
  int waitWrite(int tout);
  bool isOpen() { return this->fd_ >= 0; }
};

}

#endif


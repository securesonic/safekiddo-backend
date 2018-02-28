#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include <exception>

namespace utils
{

uint64_t getTimeMillis();
std::string sysError();

class Descriptor
{
private:
  Descriptor(const Descriptor&);
  int waitReady(int & tout, bool forRead, bool forWrite);

protected:
  int fd_;

public:
  Descriptor() : fd_(-1) {}
  explicit Descriptor(int fd) : fd_(fd) {}
  ~Descriptor() {
	  this->close();
  }

public:
  void close();

  int setNonblocking();

  int getFd() const { return this->fd_; }
  int waitRead(int & tout);
  int waitWrite(int & tout);

  bool isOpen() { return this->fd_ >= 0; }
};

} // namespace utils

#endif // __DESCRIPTOR_H__

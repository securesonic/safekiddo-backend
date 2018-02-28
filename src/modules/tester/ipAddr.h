/*
 * ip_addr.h
 *
 *  Created on: Apr 9, 2013
 *      Author: versus
 */

#ifndef IP_ADDR_H_
#define IP_ADDR_H_

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cstring>
#include <cerrno>
#include <stdexcept>
#include <string>



namespace utils {

class IpAddr {
private:
  in_addr addr_;
  in_port_t port_;

public:

  static IpAddr localhost(in_port_t port)
  {
  	return IpAddr("127.0.0.1", port);
  }

  IpAddr(const IpAddr &src)
  {
	  this->addr_ = src.addr_;
	  this->port_ = src.port_;
  }

  IpAddr()
  {
	this->addr_.s_addr = INADDR_NONE;
    this->port_ = 0;
  }

  bool empty() const
  {
  	return this->addr_.s_addr == INADDR_NONE;
  }

  explicit IpAddr(struct in_addr ip, in_port_t port)
  {
    this->addr_ = ip;
    this->port_ = port;
  }

  explicit IpAddr(in_addr_t ip, in_port_t port)
  {
  	this->addr_.s_addr = ip;
  	this->port_ = port;
  }

  explicit IpAddr(const std::string &ip, in_port_t port)
  {
    if (inet_pton(AF_INET, ip.c_str(), &this->addr_) <= 0)
    	throw std::invalid_argument(std::string("inet_pton() : ") + strerror(errno));
    this->port_ = port;
  }

  in_addr& get_addr() { return this->addr_; }
  in_port_t& get_port() { return this->port_; }

  const in_addr& get_addr() const { return this->addr_; }
  const in_port_t& get_port() const { return this->port_; }

  bool operator==(const IpAddr &sec) const
  {
    return (this->addr_.s_addr == sec.addr_.s_addr && this->port_ == sec.port_);
  }

  bool operator!=(const IpAddr &sec) const
  {
  	return !(*this == sec);
  }

  bool operator<(const IpAddr& sec) const
  {
  	return this->addr_.s_addr < sec.addr_.s_addr ||
  					(this->addr_.s_addr == sec.addr_.s_addr &&
  						this->port_ < sec.port_);
  }

  static IpAddr get_peer_addr(int fd)
  {
    struct sockaddr_in addr;

    socklen_t sa_len = sizeof(addr);
    if (getpeername(fd, reinterpret_cast<struct sockaddr*>(
    		reinterpret_cast<char*>(&addr)), &sa_len))
        throw std::invalid_argument(std::string("getpeername() : ") + strerror(errno));
    if (addr.sin_addr.s_addr == INADDR_NONE)
      throw std::invalid_argument("invalid address : " +
      		IpAddr(addr.sin_addr, addr.sin_port).get_string_addr());
    return IpAddr(addr.sin_addr, ntohs(addr.sin_port));
  }

  static IpAddr get_socket_addr(int fd) {
    struct sockaddr_in addr;
    socklen_t sa_len = sizeof(addr);
    if (getsockname(fd, reinterpret_cast<struct sockaddr*>(
    		reinterpret_cast<char*>(&addr)), &sa_len))
        throw std::invalid_argument(std::string("getsockname() : ") + strerror(errno));
    return IpAddr(addr.sin_addr, ntohs(addr.sin_port));
  }

  IpAddr& operator=(const IpAddr &ip) {
    this->addr_ = ip.addr_;
    this->port_ = ip.port_;
    return *this;
  }

  std::string get_string_addr() const {
    char str[INET_ADDRSTRLEN] = {0};
    if (!inet_ntop(AF_INET, &this->addr_, str, INET_ADDRSTRLEN))
      throw std::invalid_argument(std::string("inet_ntop() : ") + strerror(errno));
    return std::string(str);
  }
};

static inline std::ostream& operator<<(std::ostream& os, const IpAddr &ip) {
  return os << ip.get_string_addr() << " : " << ip.get_port();
}

}

#endif /* IP_ADDR_H_ */


#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <netinet/tcp.h>
#include <memory>

#include "tcpSocket.h"

using namespace std;

namespace utils {

std::ostream &operator<<(std::ostream &os, TcpSocket &sock) {
	return os << reinterpret_cast<const void*>(&sock) << " " << sock.getAddr()
			<< ", peer : " << sock.getPeer();
}

TcpSocket& TcpSocket::operator=(TcpSocket &src)
{
	  this->peer_ = src.peer_;
	  this->fd_ = src.fd_;
	  src.fd_ = -1;
	  return *this;
}

TcpSocket::TcpSocket(int fd)
	: Socket(fd)
{
}

TcpSocket::TcpSocket() throw (ios_base::failure)
{
	if ((this->fd_ = ::socket(AF_INET, SOCK_STREAM, 0)) == -1)
		throw ios_base::failure("socket()");
}

IpAddr TcpSocket::getPeer()
{
	if (this->peer_.empty())
		this->peer_ = IpAddr::get_peer_addr(this->fd_);
	return this->peer_;
}

IpAddr TcpSocket::getAddr() const
{
  return IpAddr::get_socket_addr(this->fd_);
}

int TcpSocket::getFd() const { return this->fd_; }

int TcpSocket::connect(const IpAddr &addr)
{
	int yes = 1;
	if (setsockopt(this->fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))
			|| setsockopt(this->fd_, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes))) {
		perror("setsockopt()");
		this->close();
		return errno;
	}

	assert(yes);

	struct sockaddr_in sock_addr;
	bzero(&sock_addr, sizeof(addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(addr.get_port());
	sock_addr.sin_addr = addr.get_addr();

	while (true)
	{
		int ret = ::connect(this->fd_, reinterpret_cast<struct sockaddr*>(reinterpret_cast<char*>(&sock_addr)),
				sizeof(sock_addr));
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			return errno;
		}
		break;
	}
	return 0;
}

ssize_t TcpSocket::read(void *buf, size_t len)
{
	ssize_t ret = -1;
	while (true)
	{
		ret = ::read(this->fd_, buf, len);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			return -errno;
		}
		break;
	}
	return ret;
}

ssize_t TcpSocket::write(const void *buf, size_t len)
{
	ssize_t ret = -1;
	while (true)
	{
		ret = ::write(this->fd_, buf, len);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			return -errno;
		}
		break;
	}
	return ret;
}

int TcpSocket::readAll(void *buf, size_t len)
{
	size_t done = 0;
	ssize_t r = -1;

	while (done != len)
	{
		assert(done < len);
		r = this->read(reinterpret_cast<char*>(buf) + done, len - done);
		if (r < 0)
			return -errno;
		if (!r)
			return -ENOTCONN;
		done += r;
	}
	return 0;
}

int TcpSocket::writeAll(const void *buf, size_t len)
{
	size_t done = 0;
	ssize_t w = -1;

	while (done != len)
	{
		assert(done < len);
		w = this->write(reinterpret_cast<const char*>(buf) + done, len - done);
		if (w < 0)
			return -errno;
		if (!w)
			return -ENOTCONN;
		done += w;
	}
	return 0;
}

}


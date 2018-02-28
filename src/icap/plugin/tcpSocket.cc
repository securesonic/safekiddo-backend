
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

namespace utils
{

std::ostream &operator<<(std::ostream & os, TcpSocket & sock)
{
	return os << reinterpret_cast<const void*>(&sock) << " " << sock.getAddr()
			<< ", peer : " << sock.getPeer();
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

int TcpSocket::connect(const IpAddr & addr)
{
	int const yes = 1;
	if (setsockopt(this->fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) ||
		setsockopt(this->fd_, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)))
	{
		perror("setsockopt()");
		const int errno_ = errno;
		this->close();
		return -errno_;
	}

	assert(yes);

	union {
	    struct sockaddr sa;
	    struct sockaddr_in sa_in;
	    struct sockaddr_in6 sa_in6;
	    struct sockaddr_storage sa_stor;
	} s_addr;

	bzero(&s_addr, sizeof(s_addr));
	s_addr.sa_in.sin_family = AF_INET;
	s_addr.sa_in.sin_port = htons(addr.get_port());
	s_addr.sa_in.sin_addr = addr.get_addr();


	while (true)
	{
		int ret = ::connect(this->fd_, &s_addr.sa, sizeof(s_addr.sa_in));
		if (ret < 0)
		{
			if (errno == EINTR)
				continue;
			return -errno;
		}
		break;
	}
	return 0;
}

ssize_t TcpSocket::read(void * const buf, size_t const len)
{
	ssize_t ret = -1;
	while (true)
	{
		ret = ::read(this->fd_, buf, len);
		if (ret < 0)
		{
			if (errno == EINTR)
				continue;
			return -errno;
		}
		break;
	}
	return ret;
}

ssize_t TcpSocket::write(const void * const buf, size_t const len)
{
	ssize_t ret = -1;
	while (true)
	{
		ret = ::write(this->fd_, buf, len);
		if (ret < 0)
		{
			if (errno == EINTR)
				continue;
			return -errno;
		}
		break;
	}
	return ret;
}

int TcpSocket::readAll(void * const buf, size_t const len)
{
	size_t done = 0;
	ssize_t r = -1;

	while (done != len)
	{
		assert(done < len);
		r = this->read(static_cast<char*>(buf) + done, len - done);
		if (r < 0)
			return -errno;
		if (!r)
			return -ENOTCONN;
		done += r;
	}
	return 0;
}

int TcpSocket::writeAll(const void * const buf, size_t const len)
{
	size_t done = 0;
	ssize_t w = -1;

	while (done != len)
	{
		assert(done < len);
		w = this->write(static_cast<const char*>(buf) + done, len - done);
		if (w < 0)
			return -errno;
		if (!w)
			return -ENOTCONN;
		done += w;
	}
	return 0;
}

} // namespace utils

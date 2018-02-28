
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "serverSocket.h"

namespace utils {

ServerSocket::~ServerSocket() throw () {}

ServerSocket::ServerSocket(const IpAddr &addr)
{
	if ((this->fd_ = ::socket(PF_INET, SOCK_STREAM, 0)) == -1)
	  throw std::ios_base::failure("socket()");
	int yes = 1;
	if (setsockopt(this->fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
	  throw std::ios_base::failure("setsockopt()");
	struct sockaddr_in my_addr;

	bzero(&my_addr, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(addr.get_port());
	my_addr.sin_addr = addr.get_addr();

	if (bind(this->fd_, reinterpret_cast<struct sockaddr*>(
			reinterpret_cast<char*>(&my_addr)), sizeof(my_addr)) == -1)
	{
		this->close();
		throw std::ios_base::failure("bind()");
	}

	const int qlen = 10;
	if (listen(this->fd_, qlen) == -1)
	  throw std::ios_base::failure("listen()");
}


int ServerSocket::accept()
{
	struct sockaddr_in addr;
	socklen_t clen = sizeof(addr);
	int sock;

	while (true)
	{
		sock = ::accept(this->fd_, reinterpret_cast<struct sockaddr*>(
				reinterpret_cast<char*>(&addr)), &clen);
		if (sock < 0)
		{
			if (errno == EINTR)
				continue;
			return -errno;
		}
		break;
	}
	return sock;
}


}


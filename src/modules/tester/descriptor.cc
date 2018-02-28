#include <cstdlib>
#include <poll.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <cstdio>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

using namespace std;

#include "descriptor.h"

namespace utils {

void Descriptor::close()
{
	if (this->fd_ != -1 && ::close(this->fd_))
	{
		perror("close()");
		abort();
	}
	this->fd_ = -1;
}

int Descriptor::setNonblocking()
{
	if (fcntl(this->fd_, F_SETFL, fcntl(this->fd_, F_GETFL) | O_NONBLOCK) == -1)
		return errno;
	return 0;
}




int Descriptor::waitRead(int timeout)
{
	struct pollfd pfd;
	pfd.fd = this->fd_;
	pfd.events = POLLIN;
	pfd.revents = 0;
	int ret = -1;

	while (true) {
		if (!timeout || !(ret = poll(&pfd, 1, timeout)))
			return -1;

		if (ret < 0) {
			if (errno == EINTR) continue;
			return errno;
		}

		assert(ret == 1);
		if ((pfd.revents & POLLERR) || (pfd.revents & POLLHUP))
			abort(); // todo
		assert(pfd.revents & POLLIN);
		break;
	}
	return 0;
}

int Descriptor::waitWrite(int timeout)
{
	struct pollfd pfd;
	pfd.fd = this->fd_;
	pfd.events = POLLOUT;
	pfd.revents = 0;
	int ret = -1;

	while (true) {
		if (!timeout || !(ret = poll(&pfd, 1, timeout)))
			return -1;

		if (ret < 0) {
			if (errno == EINTR) continue;
			return errno;
		}

		if (ret != 1) abort();
		if ((pfd.revents & POLLERR) || (pfd.revents & POLLHUP))
			abort(); // todo
		assert(pfd.revents & POLLOUT);
		break;
	}
	return 0;
}

}


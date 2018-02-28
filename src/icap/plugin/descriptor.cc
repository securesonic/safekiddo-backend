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

#include "descriptor.h"
#include "utils/assert.h"


namespace utils
{

uint64_t getTimeMillis()
{
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts))
	{
		FAILURE("clock_gettime failed");
	}
	return ts.tv_nsec / 1000000 + ts.tv_sec * 1000ULL;
}

std::string sysError()
{
	std::string buf;
	buf.resize(1024);
	strerror_r(errno, &buf[0], buf.size());
	buf.resize(strlen(&buf[0]));
	return buf;
}

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
	{
		return -errno;
	}
	return 0;
}

/*
 * Returns:
 *  1   if descriptor is ready
 *  0   on timeout
 * < 0  on error
 */
int Descriptor::waitReady(int & time, bool forRead, bool forWrite)
{
	assert(forRead || forWrite);
	struct pollfd pfd;
	pfd.fd = this->fd_;
	pfd.events = (forRead ? POLLIN : 0) | (forWrite ? POLLOUT : 0);
	pfd.revents = 0;
	int ret = -1;
	uint64_t start = getTimeMillis();

	while (true)
	{
		if (!(ret = poll(&pfd, 1, time)))
		{
			time = 0;
			return 0;
		}

		uint64_t end = getTimeMillis();
		time -= std::min<uint64_t>(time, end - start);
		start = end;

		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			return -errno;
		}

		assert(ret == 1);
		if ((pfd.revents & POLLERR) || (pfd.revents & POLLHUP))
		{
			return -1;
		}
		assert((pfd.revents & POLLIN) || (pfd.revents & POLLOUT));
		break;
	}
	return 1;
}

int Descriptor::waitRead(int & tout)
{
	return this->waitReady(tout, true, false);
}

int Descriptor::waitWrite(int & tout)
{
	return this->waitReady(tout, false, true);
}

} // namespace utils

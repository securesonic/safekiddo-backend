/*
 * socket.h
 *
 *  Created on: Feb 2, 2014
 *      Author: konrad
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <cerrno>



#include "descriptor.h"

namespace utils {

class Socket : public Descriptor
{
public:
	Socket() {}
	Socket(int fd) : Descriptor(fd) {}
	~Socket() throw () {
		this->close();
	}

	int setNodelay() {
	    int i = 1;
	    if (setsockopt(this->fd_, IPPROTO_TCP, TCP_NODELAY, &i, sizeof(i)) < 0)
	    	return errno;
	    return 0;
	}
};

}


#endif /* SOCKET_H_ */

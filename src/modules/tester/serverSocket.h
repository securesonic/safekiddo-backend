#ifndef __SERVER_SOCKET_H__
#define __SERVER_SOCKET_H__

#include <netinet/in.h>
#include <iostream>

#include "socket.h"
#include "ipAddr.h"

namespace utils {

class ServerSocket : public Socket
{
private:
	ServerSocket(const ServerSocket&);
	ServerSocket& operator=(const ServerSocket&);

public:
	ServerSocket(const IpAddr &addr);
	int accept();
	~ServerSocket() throw ();
};

}

#endif


#ifndef _SAFEKIDDO_SOCKETUTILS_H_
#define _SAFEKIDDO_SOCKETUTILS_H_

#include <sys/socket.h>

void safeSetsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);

#endif /* _SAFEKIDDO_SOCKETUTILS_H_ */

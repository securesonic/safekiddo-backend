#include "utils/socketUtils.h"
#include "utils/alerts.h"

#include <errno.h>

void safeSetsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
	if (::setsockopt(sockfd, level, optname, optval, optlen))
	{
		int const err = errno;
		ALERT("safeSetsockopt failed", err, sockfd, level, optname);
	}
}

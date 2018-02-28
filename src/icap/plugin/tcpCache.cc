/*
 * tcp_cache.cc
 *
 *  Created on: Aug 23, 2013
 *      Author: versus
 */

#include <ctime>
#include <netdb.h>

#include "tcpCache.h"
#include "utils/assert.h"


namespace utils
{

TcpCache::TcpCache(const std::string &host, unsigned port, int lifetime, int conn_tout)
	: host_(host),
	  port_(port),
	  lifetime_(lifetime),
	  conn_tout_(conn_tout)
{
}

static int getHostByName(const std::string &host, utils::IpAddr &ip)
{
	struct hostent hbuf;
	const size_t bufLen = 1024;
	char buf[bufLen];
	struct hostent *result = NULL;
	int err = 0;
	if (gethostbyname_r(host.c_str(), &hbuf, buf, bufLen, &result, &err))
	{
		std::cerr << "Problem with gethostbyname_r(): " << strerror(err) << "\n";
		return -1;
	}
	struct in_addr **list = (struct in_addr **)result->h_addr_list;
	if (!list[0])
	{
		std::cerr << "Invalid data from gethostbyname()\n";
		return -1;
	}

	ip = utils::IpAddr(list[0]->s_addr, ip.get_port());
	return 0;
}

boost::shared_ptr<TcpSocket> TcpCache::createNewConnection()
{
	utils::IpAddr ip = utils::IpAddr::localhost(this->port_);
	if (getHostByName(this->host_, ip))
	{
		return boost::shared_ptr<TcpSocket>();
	}
	boost::shared_ptr<TcpSocket> sock(new TcpSocket);
	sock->setNonblocking();

	int conn_tout = this->conn_tout_;
	while (true)
	{
		int const connRes = sock->connect(ip);
		switch (connRes)
		{
		case 0:
		case -EISCONN:
			return sock;
		case -EINPROGRESS:
		{
			int const res = sock->waitWrite(conn_tout);
			if (res == 1)
			{
				continue;
			}
			if (res == 0)
			{
				LOG(ERROR, "Timeout in connect(): " << sysError());
				return boost::shared_ptr<TcpSocket>();
			}
			// fall through
		}
		default:
			LOG(ERROR, "Error in connect(): " << sysError());
			return boost::shared_ptr<TcpSocket>();
		}
	}
}

boost::shared_ptr<TcpSocket> TcpCache::connect()
{

	{
		boost::mutex::scoped_lock lock(this->mtx_);
		uint64_t now = getTimeMillis();
		while (!this->list_.empty())
		{
			boost::shared_ptr<TcpSocket> sock = this->list_.front().sock;
			if (this->list_.front().validTill > now)
			{
				this->list_.pop_front();
				return sock;
			}
			this->list_.pop_front();
		}
	}

	return createNewConnection();
}

void TcpCache::add(boost::shared_ptr<TcpSocket> & sock)
{
	{
		boost::mutex::scoped_lock lock(this->mtx_);
		this->list_.push_back(cache_entry_(sock, getTimeMillis() + this->lifetime_));
	}
	sock.reset();
}

} // namespace utils

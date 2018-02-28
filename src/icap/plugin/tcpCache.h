/*
 * tcp_cache.h
 *
 *  Created on: Aug 23, 2013
 *      Author: versus
 */

#ifndef TCP_CACHE_H_
#define TCP_CACHE_H_

#include <list>

#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>


#include "tcpSocket.h"
#include "ipAddr.h"

namespace utils
{

class TcpCache : public boost::noncopyable {
public:
	TcpCache(const std::string &host, unsigned port, int lifetime, int conn_tout);

	boost::shared_ptr<TcpSocket> connect();
	void add(boost::shared_ptr<TcpSocket> & sock);

	std::string const & getHostName() const
	{
		return this->host_;
	}

private:
	boost::shared_ptr<TcpSocket> createNewConnection();

	boost::mutex mtx_;
	struct cache_entry_ {
		cache_entry_(boost::shared_ptr<TcpSocket> & sock, uint64_t validTill)
		: sock(sock),
		  validTill(validTill)
		{
		}
		boost::shared_ptr<TcpSocket> sock;
		uint64_t validTill;
	};
	std::list<cache_entry_> list_;
	const std::string host_;
	const unsigned port_;
	int lifetime_, conn_tout_;
};

} // namespace utils

#endif /* TCP_CACHE_H_ */

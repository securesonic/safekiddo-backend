#ifndef _SAFEKIDDO_UTILS_HTTPSENDER_H
#define _SAFEKIDDO_UTILS_HTTPSENDER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

using boost::asio::ip::tcp;

namespace ssl = boost::asio::ssl;
typedef ssl::stream<tcp::socket> sslSocket_t;

namespace safekiddo
{
namespace utils
{

class HttpSender
{
public:
	HttpSender(std::string const & server, bool isSsl = false);
	/* not thread safe */
	bool sendRequest(
		std::string const & path,
		std::string const & method,
		std::vector<std::string> const & otherHeaders,
		std::string const & body,
		std::vector<std::string> & responseHeaders,
		std::string & responseBody
	);

private:
	boost::asio::io_service io_service;
	std::string const server;
	bool const isSsl;
	ssl::context context;

	bool socketConnect(tcp::socket & socket, sslSocket_t & sslSocket);
	void socketClose(tcp::socket & socket, sslSocket_t & sslSocket);
	std::string const getProtocolPrefix();

	std::size_t write(tcp::socket & socket, sslSocket_t & sslSocket, boost::asio::streambuf & buffer);
	std::size_t read(
		tcp::socket & socket,
		sslSocket_t & sslSocket,
		boost::asio::streambuf & buffer,
		boost::asio::detail::transfer_at_least_t detail
	);
	std::size_t read_until(
		tcp::socket & socket,
		sslSocket_t & sslSocket,
		boost::asio::streambuf & buffer,
		std::string const & delim
	);
};

} // namespace utils
} // namespace safekiddo

#endif // _SAFEKIDDO_UTILS_HTTPSENDER_H

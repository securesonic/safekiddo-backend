#include "utils/assert.h"
#include "utils/httpSender.h"
#include "utils/loggingDevice.h"
#include "utils/stringFunctions.h"

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

namespace safekiddo
{
namespace utils
{

HttpSender::HttpSender(std::string const & server, bool isSsl):
	server(server),
	isSsl(isSsl),
	context(boost::asio::ssl::context::sslv23)
{
	if(this->isSsl)
	{
		this->context.set_default_verify_paths();
	}
}

bool HttpSender::socketConnect(tcp::socket & socket, sslSocket_t & sslSocket)
{
	try
	{
		tcp::resolver resolver(this->io_service);
		if(this->isSsl)
		{
			// Open a socket and connect it to the remote host.
			tcp::resolver::query query(this->server, "https");
			boost::asio::connect(sslSocket.lowest_layer(), resolver.resolve(query));
			sslSocket.lowest_layer().set_option(tcp::no_delay(true));

			// Perform SSL handshake and verify the remote host's
			// certificate.
			sslSocket.set_verify_mode(ssl::verify_peer);
			sslSocket.set_verify_callback(ssl::rfc2818_verification(this->server));
			sslSocket.handshake(ssl::stream<tcp::socket>::client);
		}
		else
		{
			tcp::resolver::query query(this->server, "http");
			boost::asio::connect(socket, resolver.resolve(query));
		}
		return true;
	}
	catch (boost::system::system_error& e)
	{
		LOG(ERROR, "HttpSender::connect exception: ", e.what());
	}
	return false;

}
void HttpSender::socketClose(tcp::socket & socket, sslSocket_t & sslSocket)
{
	try
	{
		if(this->isSsl)
		{
			sslSocket.lowest_layer().close();
		}
		else
		{
			socket.close();
		}
	}
	catch(boost::system::system_error& e)
	{
		LOG(ERROR, "HttpSender::close error: ", e.what());
	}
}

std::string const HttpSender::getProtocolPrefix()
{
	if(this->isSsl)
	{
		return "https://";
	}
	return "http://";
}

bool HttpSender::sendRequest(
	std::string const & path,
	std::string const & method,
	std::vector<std::string> const & otherHeaders,
	std::string const & body,
	std::vector<std::string> & responseHeaders,
	std::string & responseBody
)
{
	DLOG(1, "HttpSender: issuing request", method, path);
	try
	{
		tcp::socket socket(this->io_service);
		sslSocket_t sslSocket(this->io_service, this->context);

		if(!this->socketConnect(socket, sslSocket))
		{
			LOG(ERROR, "HttpSender: connect unsuccessful", method, path);
			return false;
		}

		// Form the request. We specify the "Connection: close" header so that the
		// server will close the socket after transmitting the response. This will
		// allow us to treat all data up until the EOF as the content.
		std::string const hostHeaderPrefix = "Host: ";
		std::string const contentLengthHeaderPrefix = "Content-Length: ";
		std::string const connectionHeaderPrefix = "Connection: ";
		std::string const contentTypeHeaderPrefix = "Content-Type: ";

		boost::asio::streambuf request;
		std::ostream requestStream(&request);
		requestStream << method << " " << path << " HTTP/1.0\r\n";
		requestStream << hostHeaderPrefix << this->server << "\r\n";
		requestStream << contentLengthHeaderPrefix << body.length() << "\r\n";
		requestStream << contentTypeHeaderPrefix << "application/x-www-form-urlencoded" << "\r\n";

		for(auto &header : otherHeaders)
		{
			DASSERT(!isPrefixOfNoCase(hostHeaderPrefix, header), "Host header declared two times");
			DASSERT(!isPrefixOfNoCase(contentLengthHeaderPrefix, header), "Content-Length header declared two times");
			DASSERT(!isPrefixOfNoCase(contentTypeHeaderPrefix, header), "Content-Type header declared two times");
			DASSERT(!isPrefixOfNoCase(connectionHeaderPrefix, header), "Connection header declared two times");
			requestStream << header << "\r\n";
		}

		requestStream << connectionHeaderPrefix << "close\r\n\r\n";
		requestStream << body;

		this->write(socket, sslSocket, request);

		// Read the response status line.
		boost::asio::streambuf response;
		this->read_until(socket, sslSocket, response, "\r\n");

		// Check that response is OK.
		std::istream responseStream(&response);
		std::string httpVersion;
		responseStream >> httpVersion;
		unsigned int statusCode;
		responseStream >> statusCode;
		std::string statusMessage;
		std::getline(responseStream, statusMessage);
		if (!responseStream || !isPrefixOfNoCase("HTTP/", httpVersion))
		{
			LOG(ERROR, "HttpSender: response error - invalid response", method, path);
			return false;
		}
		if (statusCode != 200)
		{
			LOG(ERROR, "HttpSender: response returned with status code " << statusCode, method, path);
			return false;
		}

		// Read the response headers, which are terminated by a blank line.
		this->read_until(socket, sslSocket, response, "\r\n\r\n");

		// Process the response headers.
		std::string header;
		responseHeaders.clear();
		boost::optional<size_t> contentLength;
		while (std::getline(responseStream, header))
		{
			if (!header.empty() && header[header.size() - 1] == '\r')
			{
				header.resize(header.size() - 1);
			}
			if (header.empty())
			{
				break;
			}
			DLOG(2, "response header: '" << header << "'");

			responseHeaders.push_back(header);
			if(isPrefixOfNoCase(contentLengthHeaderPrefix, header))
			{
				//Removing "Content-Length: " prefix
				std::string const contentLengthStr = header.substr(contentLengthHeaderPrefix.length());
				try
				{
					contentLength = boost::lexical_cast<size_t>(contentLengthStr);
				}
				catch (boost::bad_lexical_cast &)
				{
					LOG(ERROR, "HttpSender: bad Content-Length '" << contentLengthStr << "'", method, path);
					return false;
				}
			}
		}

		if (!contentLength)
		{
			DLOG(1, "HttpSender: no Content-Length, ignoring body", method, path);
			responseBody.clear();
		}
		else
		{
			DLOG(1, "HttpSender: contentLength=" << contentLength.get(), method, path);
			if (contentLength.get() != response.size())
			{
				if (contentLength.get() > response.size())
				{
					DLOG(1, "HttpSender: need to receive more data", response.size(), method, path);
					this->read(socket, sslSocket, response, boost::asio::transfer_at_least(contentLength.get() - response.size()));
				}
				if(response.size() != contentLength.get())
				{
					LOG(ERROR, "HttpSender: body length is longer than Content-Length", method, path);
					return false;
				}
			}

			responseBody.resize(contentLength.get());
			responseStream.read(&responseBody[0], contentLength.get());
		}

		this->socketClose(socket, sslSocket);
	}
	catch (boost::system::system_error & e)
	{
		LOG(ERROR, "HttpSender: sendRequest exception", e.what(), method, path);
		return false;
	}
	LOG(INFO, "HttpSender: request successful", method, path);
	return true;
}

std::size_t HttpSender::write(tcp::socket & socket, sslSocket_t & sslSocket, boost::asio::streambuf & buffer)
{
	if(this->isSsl)
	{
		return boost::asio::write(sslSocket, buffer);
	}
	return boost::asio::write(socket, buffer);
}

std::size_t HttpSender::read(
	tcp::socket & socket,
	sslSocket_t & sslSocket,
	boost::asio::streambuf & buffer,
	boost::asio::detail::transfer_at_least_t detail
)
{
	if(this->isSsl)
	{
		return boost::asio::read(sslSocket, buffer, detail);
	}
	return boost::asio::read(socket, buffer, detail);
}

std::size_t HttpSender::read_until(
	tcp::socket & socket,
	sslSocket_t & sslSocket,
	boost::asio::streambuf & buffer,
	std::string const & delim
)
{
	if(this->isSsl)
	{
		return boost::asio::read_until(sslSocket, buffer, delim);
	}
	return boost::asio::read_until(socket, buffer, delim);
}

} // namespace utils
} // namespace safekiddo


#include <microhttpd.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <mutex>
#include <memory>

#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <zmq.h>

#include "proto/safekiddo.pb.h"
#include "proto/utils.h"

#include "utils/logMacro.h"
#include "utils/loggingDevice.h"
#include "utils/assert.h"
#include "utils/foreach.h"
#include "utils/timeUtils.h"
#include "httpdMgmt.h"
#include "utils/alerts.h"
#include "version/version.h"

#include "admissionControlContainer.h"
#include "serverStatus.h"


namespace su = safekiddo::utils;

namespace safekiddo
{
namespace webservice
{

using protocol::messages::Request;
using protocol::messages::Response;

bool parseZmqResponse(zmq_msg_t * msg, Response & response)
{
	bool const ret = response.ParseFromArray(zmq_msg_data(msg), zmq_msg_size(msg));
	if (!ret)
	{
		LOG(ERROR, "cannot parse zmq response", zmq_msg_size(msg));
	}
	return ret;
}

void createZmqRequest(Request const & request, zmq_msg_t * msg)
{
	size_t const requestSize = request.ByteSize();
	int const rc = zmq_msg_init_size(msg, requestSize);

	ERRNO_ASSERT(rc == 0, "cannot create zmq message");
	request.SerializeWithCachedSizesToArray(static_cast<uint8_t*>(zmq_msg_data(msg)));
}

// FIXME: move to separate file
// thread-safe
class SocketPool
{
public:
	SocketPool(
		void * context,
		char const * clientsAddress,
		int32_t maxSockets
	);

	~SocketPool();

	void * getSocket();
	void returnSocket(void * socket);
	void destroySocket(void * socket);

private:
	int32_t numSockets; // number of existing sockets
	std::list<void *> socketList;
	boost::mutex mutable mutex;
	boost::condition_variable mutable socketAvailable;

	void * context;
	char const * clientsAddress;
	int32_t const maxSockets;
};

SocketPool::SocketPool(
	void * context,
	char const * clientsAddress,
	int32_t maxSockets
):
	numSockets(0),
	socketList(),
	mutex(),
	socketAvailable(),
	context(context),
	clientsAddress(clientsAddress),
	maxSockets(maxSockets)
{
}

SocketPool::~SocketPool()
{
	boost::mutex::scoped_lock lock(this->mutex);
	ASSERT((size_t)this->numSockets == this->socketList.size(),
			"some sockets were not returned", this->numSockets,
		this->socketList.size());
	FOREACH_CREF(socket, this->socketList)
	{
		int32_t rc = zmq_close(socket);
		ERRNO_ASSERT(rc == 0, "zmq_close failed", socket);
	}
}

void * SocketPool::getSocket()
{
	boost::mutex::scoped_lock lock(this->mutex);
	if (this->socketList.empty())
	{
		if (this->numSockets < this->maxSockets)
		{
			void * socket = zmq_socket(this->context, ZMQ_REQ);
			if (socket)
			{
				int32_t curNumSockets = ++this->numSockets;
				lock.unlock();
				DLOG(1, "created new socket", socket, curNumSockets);
				int32_t rc = zmq_connect(socket, this->clientsAddress);
				ERRNO_ASSERT(rc == 0, "zmq_connect failed", socket, this->clientsAddress);
				return socket;
			}
		}
		if (this->numSockets == 0)
		{
			// zmq sockets were closed by zmq_close() but their cleanup is only possible when recv is called on ZMQ_ROUTER socket
			return NULL;
		}
		DLOG(1, "waiting for socket available", this->numSockets);
		while (this->socketList.empty())
		{
			this->socketAvailable.wait(lock);
		}
		if (this->socketList.size() > 1)
		{
			// we were woken up in returnSocket() but some other thread might have sneaked into the mutex before us and returned another
			// socket without waking up another waiter
			this->socketAvailable.notify_one();
		}
		DLOG(1, "socket now available");
	}
	void * socket = this->socketList.front();
	this->socketList.pop_front();
	return socket;
}

void SocketPool::returnSocket(void * socket)
{
	boost::mutex::scoped_lock lock(this->mutex);
	bool wasEmpty = this->socketList.empty();
	this->socketList.push_front(socket);
	if (wasEmpty)
	{
		this->socketAvailable.notify_one();
	}
}

void SocketPool::destroySocket(void * socket)
{
	boost::mutex::scoped_lock lock(this->mutex);
	ASSERT(this->numSockets > 0, "numSockets underflow");
	int32_t curNumSockets = --this->numSockets;
	lock.unlock();
	DLOG(1, "destroying socket", socket, curNumSockets);
	int32_t rc = zmq_close(socket);
	ERRNO_ASSERT(rc == 0, "zmq_close failed", socket);
}


struct HttpdParams
{
	int32_t requestTimeout;
	SocketPool * socketPool;
	std::shared_ptr<ServerStatus> status;
	std::shared_ptr<AdmissionControlContainer> admissionControlContainer;
	bool useForwardedFor;
};

// FIXME: there is another BackendQueryResult used in backend; rename?
class BackendQueryResult
{
public:
	typedef boost::optional<std::pair<uint32_t, std::string> > profile_type;
	typedef boost::optional<std::pair<uint32_t, std::string> > category_group_type;

	explicit BackendQueryResult(Response::Result code)
		: code(code)
		, profile()
		, categoryGroup()
		, timeout(false)
	{
	}

	void setCode(Response::Result code)
	{
		this->code = code;
	}

	void setProfile(uint32_t id, std::string const & name)
	{
		this->profile = std::make_pair(id, name);
	}

	void setCategoryGroup(uint32_t id, std::string const & name)
	{
		this->categoryGroup = std::make_pair(id, name);
	}

	void setTimeout()
	{
		this->timeout = true;
	}

	Response::Result getCode() const
	{
		return this->code;
	}

	profile_type const & getProfile() const
	{
		return this->profile;
	}

	category_group_type const & getCategoryGroup() const
	{
		return this->categoryGroup;
	}

	bool isTimeout() const
	{
		return this->timeout;
	}

private:
	Response::Result code;
	profile_type profile;
	category_group_type categoryGroup;
	bool timeout;
};

// TODO: thread should block signals
boost::shared_ptr<const BackendQueryResult> backendQuery(
	char const * url,
	char const * userId,
	Request::UserActionCode const userAction,
	HttpdParams const * params
)
{
	Request rq;
	rq.set_url(url);
	rq.set_userid(userId);
	rq.set_useraction(userAction);

	DLOG(1, "querying backend", rq);

	boost::shared_ptr<BackendQueryResult> result(new BackendQueryResult(Response::RES_OK));

	void * socket = params->socketPool->getSocket();
	if (!socket)
	{
		// If backend is not available then messages are not read from ZMQ_ROUTER socket, which is necessary for zmq to cleanup a socket
		// after zmq_close() is called. When backend is back sockets should be available again.
		// Pretend that BE request timed out.
		LOG(WARNING, "zmq socket is not available, probably backend is down", rq);
		result->setTimeout();
		return result;
	}

	zmq_msg_t zmqRequest;
	createZmqRequest(rq, &zmqRequest);

	int32_t rc = zmq_msg_send(&zmqRequest, socket, 0);
	ERRNO_ASSERT(rc != -1, "zmq_msg_send error");
	rc = zmq_msg_close(&zmqRequest);
	ERRNO_ASSERT(rc == 0, "zmq_msg_close error");

	DLOG(3, "polling for reply with timeout=" << params->requestTimeout);
	// sent request; wait for reply
	zmq_pollitem_t items [] = { { socket, 0, ZMQ_POLLIN, 0 } };
	rc = zmq_poll(items, 1, params->requestTimeout);
	ERRNO_ASSERT(rc != -1, "interrupted zmq_poll");

	if (items[0].revents & ZMQ_POLLIN)
	{
		zmq_msg_t zmqResponse;
		rc = zmq_msg_init(&zmqResponse);
		ERRNO_ASSERT(rc == 0, "cannot create zmq message");

		rc = zmq_msg_recv(&zmqResponse, socket, 0);
		ERRNO_ASSERT(rc != -1, "zmq_msg_recv error");

		Response rs;

		bool const parseOk = parseZmqResponse(&zmqResponse, rs);
		rc = zmq_msg_close(&zmqResponse);
		ERRNO_ASSERT(rc == 0, "zmq_msg_close error");

		if (!parseOk)
		{
			ALERT("cannot parse zmq response for query: " << rq);
		}
		else
		{
			result->setCode(rs.result());

			if (rs.has_usedprofileid())
			{
				ASSERT(rs.has_usedprofilename(), "used profile name not found");
				result->setProfile(rs.usedprofileid(), rs.usedprofilename());
			}

			if (rs.has_categorygroupid())
			{
				ASSERT(rs.has_categorygroupname(), "category group name not found");
				result->setCategoryGroup(rs.categorygroupid(), rs.categorygroupname());
			}
		}
	}
	else
	{
		LOG(WARNING, "backend did not respond within timeout=" << params->requestTimeout, rq);
		result->setTimeout();
		// destroy socket, it's of no use any longer
		params->socketPool->destroySocket(socket);
		socket = NULL;
	}

	if (socket)
	{
		params->socketPool->returnSocket(socket);
	}

	return result;
}

// returns empty option for invalid string
boost::optional<Request::UserActionCode> parseUserAction(char const * userActionStr)
{
	int32_t userActionNum = -1;
	try
	{
		userActionNum = boost::lexical_cast<int32_t>(userActionStr);
	}
	catch (boost::bad_lexical_cast &)
	{
		DLOG(1, "user action should be numeric", userActionStr);
		return boost::none;
	}
	switch (userActionNum)
	{
	case Request::NON_INTENTIONAL_REQUEST:
	case Request::INTENTIONAL_REQUEST:
	case Request::CATEGORY_GROUP_QUERY:
		break;
	default:
		DLOG(1, "unknown user action number: " << userActionNum);
		return boost::none;
	}
	return static_cast<Request::UserActionCode>(userActionNum);
}

bool isValidBackendResult(Response::Result resultCode)
{
	switch (resultCode)
	{
	case Response::RES_UNKNOWN_USER:
	case Response::RES_INTERNET_ACCESS_FORBIDDEN:
	case Response::RES_CATEGORY_BLOCKED_DEFAULT:
	case Response::RES_CATEGORY_BLOCKED_CUSTOM:
	case Response::RES_CATEGORY_BLOCKED_GLOBAL:
	case Response::RES_URL_BLOCKED_DEFAULT:
	case Response::RES_URL_BLOCKED_CUSTOM:
	case Response::RES_URL_BLOCKED_GLOBAL:
	case Response::RES_IP_REPUTATION_CHECK_FAILED:
	case Response::RES_OK:
		return true;
	}
	return false;
}

static std::string getLastIp(std::string const & forwardedFor)
{
	size_t pos = forwardedFor.rfind(',');
	pos = (pos == std::string::npos) ? 0 : pos + 1;
	size_t const start = forwardedFor.find_first_not_of(' ', pos);
	if (start != std::string::npos)
	{
		size_t const last = forwardedFor.find_last_not_of(' ');
		return forwardedFor.substr(start, last + 1 - start);
	}
	return "(invalid)";
}

static in6_addr getClientIp(
	struct MHD_Connection * connection,
	bool useForwardedFor
)
{
	char const * forwardedFor = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Forwarded-For");
	union MHD_ConnectionInfo const * info = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
	ASSERT(info, "MHD_get_connection_info failed");
	in6_addr const remoteIp = AdmissionControlContainer::inAddrV4ToV6(reinterpret_cast<struct sockaddr_in *>(info->client_addr)->sin_addr);
	DLOG(3, "got connection from " << AdmissionControlContainer::ipToString(remoteIp) << "with X-Forwarded-For: " <<
		(forwardedFor ? forwardedFor : "(NULL)"));
	if (useForwardedFor)
	{
		if (!forwardedFor)
		{
			// monit sends categorization requests from localhost
			if (!(remoteIp == AdmissionControlContainer::inAddrV4ToV6(in_addr{htonl(INADDR_LOOPBACK)})))
			{
				LOG(WARNING, "Configured to use X-Forwarded-For header but there is no one, remoteIp: " <<
					AdmissionControlContainer::ipToString(remoteIp));
			}
			return remoteIp;
		}
		in6_addr clientIp;
		if(!AdmissionControlContainer::stringToIp(getLastIp(forwardedFor), clientIp))
		{
			LOG(ERROR, "Cannot parse ip address", forwardedFor);
			return remoteIp;
		}
		return clientIp;
	}
	else
	{
		if (forwardedFor)
		{
			// Some proxy server outside of us may have added the header.
			DLOG(1, "Configured NOT to use X-Forwarded-For header but there is one, ignoring. remoteIp: " <<
				AdmissionControlContainer::ipToString(remoteIp) << ", X-Forwarded-For: " << forwardedFor);
		}
		return remoteIp;
	}
}

static unsigned int processRequestAndReturnHttpStatus(
	struct MHD_Connection * connection,
	const char * url,
	const char * method,
	HttpdParams const * params,
	struct MHD_Response * response
)
{
	char const * ok = "0";
	char const * requestUrl = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Request");
	char const * userId = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "UserId");
	char const * userActionStr = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "UserAction");
	boost::optional<Request::UserActionCode> const userActionOpt = userActionStr ? parseUserAction(userActionStr) :
		Request::INTENTIONAL_REQUEST;

	if (strcasecmp(url, "/healthcheck") == 0)
	{
		if (!params->status->statusOk())
		{
			return MHD_HTTP_SERVICE_UNAVAILABLE;
		}
		return MHD_HTTP_OK;
	}

	bool const categorizationRequest = requestUrl && userId && userActionOpt && strcmp(url, "/cc") == 0 &&
		strcasecmp(method, "GET") == 0;
	if (!categorizationRequest)
	{
		LOG(WARNING, "invalid request"
			", requestUrl: " << (requestUrl ? requestUrl : "(NULL)") <<
			", userId: " << (userId ? userId : "(NULL)") <<
			", userAction: " << (userActionStr ? userActionStr : "(NULL)") <<
			", url: " << url <<
			", method: " << method
		);
		return MHD_HTTP_BAD_REQUEST;
	}
	///////// VALID categorization request //////////

	in6_addr const clientIp = getClientIp(connection, params->useForwardedFor);
	std::string const clientIpString = AdmissionControlContainer::ipToString(clientIp);

	DLOG(1, "received request for '" << requestUrl << "', client: " << clientIpString);

	uint32_t ipAttempts;
	int const accessValue = params->admissionControlContainer->getAccess(clientIp, ipAttempts);

	if (accessValue == -1)
	{
		// log on debug level to not overfill logs
		DLOG(1, "IP Address was blocked", clientIpString, ipAttempts);
		return MHD_HTTP_SERVICE_UNAVAILABLE;
	}
	else if (accessValue > 0)
	{
		// this is throttled, so can be on non-debug level
		LOG(WARNING, "IP Address was punished with slowdown", clientIpString, ipAttempts);
		usleep(accessValue * 1000);
	}
	else
	{
		DASSERT(accessValue == 0, "unexpected accessValue: " << accessValue);
	}

	DBG_BEGIN_TIMED_BLOCK(backendQuery)
	boost::shared_ptr<const BackendQueryResult> const result = backendQuery(requestUrl, userId, userActionOpt.get(), params);
	DBG_END_TIMED_BLOCK(backendQuery)
	Response::Result resultCode = result->getCode();
	DLOG(1, "query result: " << resultCode, backendQueryDuration, requestUrl);

	if (resultCode == Response::RES_OK)
	{
		MHD_add_response_header(response, "Result", ok);
	}
	else if (!isValidBackendResult(resultCode))
	{
		// cast to avoid assertion in stream output operator (if one is added) for the Response::Result
		LOG(ERROR, "malformed result code from backend: " << static_cast<int>(resultCode));
		MHD_add_response_header(response, "Result", ok);
	}
	else
	{
		if (resultCode == Response::RES_UNKNOWN_USER)
		{
			DLOG(1, "Unknown user id: ", userId,  clientIpString);
			params->admissionControlContainer->addInvalidCredentialsPenalty(clientIp);
		}
		MHD_add_response_header(response, "Result", boost::lexical_cast<std::string>(resultCode).c_str());
	}

	if (result->getProfile())
	{
		MHD_add_response_header(response, "Used-profile-id",
				boost::lexical_cast<std::string>(result->getProfile()->first).c_str());
		MHD_add_response_header(response, "Used-profile-name",
				result->getProfile()->second.c_str());
	}

	if (result->getCategoryGroup())
	{
		MHD_add_response_header(response, "Category-group-id",
				boost::lexical_cast<std::string>(result->getCategoryGroup()->first).c_str());
		MHD_add_response_header(response, "Category-group-name",
				result->getCategoryGroup()->second.c_str());
	}

	if (result->isTimeout())
	{
		params->status->reportProblem();
	}

#ifndef NDEBUG
	if (result->isTimeout())
	{
		MHD_add_response_header(response, "X-Result", "timeout");
	}
	MHD_add_response_header(response, "Request", requestUrl);
#endif

	return MHD_HTTP_OK;
}

static int handler(
	void * cls,
	struct MHD_Connection * connection,
	const char * url,
	const char * method,
	const char * /*version*/,
	const char * /*upload_data*/,
	size_t * /*upload_data_size*/,
	void **ptr
)
{
	*ptr = NULL;
	HttpdParams const * params = static_cast<HttpdParams const *>(cls);
	struct MHD_Response * response = MHD_create_response_from_buffer(5, const_cast<char *>("hack\n"), MHD_RESPMEM_PERSISTENT);
	unsigned int httpStatus = processRequestAndReturnHttpStatus(
		connection,
		url,
		method,
		params,
		response
	);
	int ret = MHD_queue_response(connection, httpStatus, response);
	MHD_destroy_response(response);
	DLOG(3, "response sent");
	return ret;
}

} // namespace webservice
} // namespace safekiddo

namespace po = boost::program_options;
namespace ws = safekiddo::webservice;

#ifdef NDEBUG
#define DEFAULT_LOG_LEVEL	"INFO"
#else
#define DEFAULT_LOG_LEVEL	"DEBUG1"
#endif

int main(int argc, char **argv)
{
	po::options_description desc("Command line options");

	std::string backendAddress;
	uint32_t port = 0;
	int32_t requestTimeout = 0;
	uint32_t clientTimeout = 0;
	uint32_t connectionLimit = 0;

	uint32_t connectionPerIpLimitAbsolute = 0;
	uint32_t connectionPerIpLimitSlowDown = 0;
	uint32_t invalidCredentialsPenalty = 0;
	uint32_t slowDownDuration = 0;
	uint32_t admissionControlPeriod = 0;

	std::string keyFile, certFile;
	std::string logFilePath;
	std::string logLevel;
	std::string mgmtFifoPath;
	std::string admissionControlCustomSettingsFile;

	desc.add_options()
		("version,v", "Display version information")
		("help,h", "Produce help message")
		("port,p", po::value<uint32_t>(&port)->default_value(8080),
			"Server port")
		("timeout,t", po::value<int32_t>(&requestTimeout)->default_value(3000),
			"request timeout")
		("backend,b", po::value<std::string>(&backendAddress)->default_value("tcp://localhost:7777"),
			"backend address")
		("clientTimeout", po::value<uint32_t>(&clientTimeout)->default_value(30),
			"timeout for inactive client")
		("connectionLimit", po::value<uint32_t>(&connectionLimit)->default_value(300),
			"limit for number of concurrent connections")
		("connectionPerIpLimitAbsolute", po::value<uint32_t>(&connectionPerIpLimitAbsolute)->default_value(500),
			"maximum requests from one ip during one admission control period before client is blocked")
		("connectionPerIpLimitSlowDown", po::value<uint32_t>(&connectionPerIpLimitSlowDown)->default_value(300),
			"maximum requests from one ip during one admission control period before client is slowed down")
		("invalidCredentialsPenalty", po::value<uint32_t>(&invalidCredentialsPenalty)->default_value(50),
			"number of connection attempts added as penalty for trying to connect with invalid credentials (result 300)")
		("slowDownDuration", po::value<uint32_t>(&slowDownDuration)->default_value(300),
			"duration of slowdown after reaching connectionPerIpLimitSlowDown, in milliseconds")
		("admissionControlPeriod", po::value<uint32_t>(&admissionControlPeriod)->default_value(30),
			"length of admission control period, in seconds")
		("admissionControlCustomSettingsFile", po::value<std::string>(&admissionControlCustomSettingsFile)
			->default_value("admissionControlCustomSettings.accs"),"admission control custom settings file path")
		("ssl,s", "Use SSL (HTTPS)")
		("keyFile", po::value<std::string>(&keyFile)->default_value("httpd.key"),
			"Server private key file")
		("certFile", po::value<std::string>(&certFile)->default_value("httpd.pem"),
			"Server certificate file")
		("logFilePath", po::value<std::string>(&logFilePath)->default_value("httpd.log"),
				"log file path")
		("logLevel", po::value<std::string>(&logLevel)->default_value(DEFAULT_LOG_LEVEL),
				"log level")
		("mgmtFifoPath", po::value<std::string>(&mgmtFifoPath)->default_value("httpdMgmt.fifo"),
				"httpd management fifo file path")
		("disableForwardedFor", "X-Forwarded-For header will not be checked")
		;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("version"))
	{
		std::cerr << safekiddo::version::getDescription() << std::endl;
		return 1;
	}

	if (vm.count("help"))
	{
		std::cerr << desc << std::endl;
		return 1;
	}

	boost::optional<su::LogLevelEnum> logLevelEnum = su::LoggingDevice::parseLogLevel(logLevel);
	if (!logLevelEnum)
	{
		std::cerr << "Invalid logLevel argument: " << logLevel << "\n";
		return 1;
	}

	su::FileLoggingDevice loggingDevice(logFilePath, logLevelEnum.get());
	su::ScopedLoggingDeviceSetter logSetter(&loggingDevice);

	bool const useSsl = vm.count("ssl");
	bool const useForwardedFor = !vm.count("disableForwardedFor");

	void * context = zmq_ctx_new();
	ERRNO_ASSERT(context != NULL, "zmq_ctx_new failed");

	char const * clientsAddress = "inproc://httpd-internal";

	// TODO: increase ulimit for max num of descriptors in environment:
	// - zmq uses at least 2 descriptors per ZMQ_REQ inproc socket
	// - at least 1 descriptors for HTTP connection
	// TODO: We have MHD_OPTION_CONNECTION_LIMIT and each connection uses one zmq socket, so maybe the limit for number of sockets in
	// SocketPool is not needed.
	// TODO: Maybe create at once maximum number of zmq sockets, because if zmq hits out-of-descriptors error when creating a socket then it
	// fires an assertion in signaler.cpp:234.
	int32_t const maxSocketsInPool = 256;

	int32_t rc = zmq_ctx_set(context, ZMQ_MAX_SOCKETS, maxSocketsInPool + 10);
	ERRNO_ASSERT(rc == 0, "zmq_ctx_set error");

	void * router = zmq_socket(context, ZMQ_ROUTER);
	rc = zmq_bind(router, clientsAddress);
	ERRNO_ASSERT(rc == 0, "zmq_bind error");

	void * dealer = zmq_socket(context, ZMQ_DEALER);

	int32_t const lingerTime = 0;
	rc = zmq_setsockopt(dealer, ZMQ_LINGER, &lingerTime, sizeof(lingerTime));
	ERRNO_ASSERT(rc == 0, "zmq_setsockopt error");

	rc = zmq_connect(dealer, backendAddress.c_str());
	ERRNO_ASSERT(rc == 0, "zmq_connect error");

	uint32_t const hwm = 1000000;
	if (zmq_setsockopt(router, ZMQ_SNDHWM, &hwm, sizeof(hwm)) ||
		zmq_setsockopt(router, ZMQ_RCVHWM, &hwm, sizeof(hwm)) ||
		zmq_setsockopt(dealer, ZMQ_SNDHWM, &hwm, sizeof(hwm)) ||
		zmq_setsockopt(dealer, ZMQ_RCVHWM, &hwm, sizeof(hwm)))
	{
		LOG(ERROR, "zmq_setsockopt() failed");
		return 1;
	}

	std::shared_ptr<ws::ServerStatus> serverStatus(new ws::ServerStatus);
	std::shared_ptr<ws::AdmissionControlContainer> admissionControlContainer(
		new ws::AdmissionControlContainer(
			connectionPerIpLimitAbsolute,
			connectionPerIpLimitSlowDown,
			invalidCredentialsPenalty,
			slowDownDuration,
			admissionControlPeriod,
			admissionControlCustomSettingsFile
		)
	);

	ws::HttpdMgmt httpdMgmt(mgmtFifoPath, admissionControlContainer);

	// scope for SocketPool
	{
		ws::SocketPool socketPool(
			context,
			clientsAddress,
			maxSocketsInPool
		);

		// accessed by multiple threads
		ws::HttpdParams params = {
			requestTimeout,
			&socketPool,
			serverStatus,
			admissionControlContainer,
			useForwardedFor
		};

		struct MHD_Daemon * daemon = NULL;

		unsigned int flags = MHD_USE_THREAD_PER_CONNECTION | MHD_USE_POLL;
#ifndef NDEBUG
		flags |= MHD_USE_DEBUG;
#endif

		if (useSsl)
		{
			std::ifstream server_key_file(keyFile.c_str(), std::ifstream::in),
					server_cert_file(certFile.c_str(), std::ifstream::in);

			if (!server_key_file.is_open())
			{
				std::cerr << "Missing " << keyFile << " file" << std::endl;
				return 1;
			}

			if (!server_cert_file.is_open())
			{
				std::cerr << "Missing " << certFile << " file" << std::endl;
				return 1;
			}

			std::string server_key((std::istreambuf_iterator<char>(server_key_file)), (std::istreambuf_iterator<char>()));

			std::string server_cert((std::istreambuf_iterator<char>(server_cert_file)), (std::istreambuf_iterator<char>()));

			flags |= MHD_USE_SSL;

			daemon = MHD_start_daemon(flags, port, NULL, NULL,
					&ws::handler, &params,
					MHD_OPTION_CONNECTION_LIMIT, connectionLimit,
					MHD_OPTION_CONNECTION_TIMEOUT, clientTimeout,
					MHD_OPTION_HTTPS_MEM_KEY, server_key.c_str(),
					MHD_OPTION_HTTPS_MEM_CERT, server_cert.c_str(),
					MHD_OPTION_END
			);
		}
		else
		{
			daemon = MHD_start_daemon(flags, port, NULL, NULL, &ws::handler, &params,
			MHD_OPTION_CONNECTION_LIMIT, connectionLimit,
			MHD_OPTION_CONNECTION_TIMEOUT, clientTimeout,
			MHD_OPTION_END);

		}

		if (!daemon)
		{
			LOG(FATAL, "MHD_start_deamon() failed");
			return 1;
		}

		LOG(INFO, "httpd started; version: " << safekiddo::version::getDescription());

		httpdMgmt.start();

		zmq_proxy(router, dealer, NULL); // EINTR safe

		FAILURE("unreachable");

		MHD_stop_daemon(daemon);
	}

	zmq_close(router);
	zmq_close(dealer);
	zmq_ctx_destroy(context);

	return 0;
}

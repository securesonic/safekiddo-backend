/*
 *  Copyright (C) 2004-2008 Christos Tsantilas
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA.
 */

#include <cstring>
#include <cerrno>

#include <c_icap/c-icap.h>
#include <c_icap/debug.h>
#include <c_icap/service.h>
#include <c_icap/header.h>
#include <c_icap/body.h>
#include <c_icap/simple_api.h>

#include "apiConnect.h"
#include "socket.h"
#include "tcpSocket.h"
#include "tcpCache.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>

#include "utils/loggingDevice.h"
#include "utils/assert.h"
#include "utils/urlNormalization.h"
#include "utils/foreach.h"
#include "utils/stringFunctions.h"
#include "utils/alerts.h"


namespace su = safekiddo::utils;

#ifdef NDEBUG
#define DEFAULT_LOG_LEVEL	"INFO"
#else
#define DEFAULT_LOG_LEVEL	"DEBUG1"
#endif

int safekiddo_init_service(ci_service_xdata_t * srv_xdata, struct ci_server_conf *server_conf);
int safekiddo_post_init_service(ci_service_xdata_t * srv_xdata, struct ci_server_conf *server_conf);
int safekiddo_check_preview_handler(char *preview_data, int preview_data_len, ci_request_t *);
int safekiddo_end_of_data_handler(ci_request_t * req);
void *safekiddo_init_request_data(ci_request_t * req);
void safekiddo_close_service();
void safekiddo_release_request_data(void *data);
int safekiddo_io(char *wbuf, int *wlen, char *rbuf, int *rlen, int iseof, ci_request_t * req);


/***********************************************************
 * SafeKiddo plugin configuration.
 *
 * Note: variables may be used not earlier than safekiddo_post_init_service().
 **********************************************************/
// LoggingDevice
static char const * confLogLevel = DEFAULT_LOG_LEVEL;
static char const * confLogFile = NULL;

// TcpCache
static char const * wsUrlAddress = NULL;
static int wsUrlPort = 80;
static int tcpCacheInactivityTime = 10000; // ms; must be lower than clientTimeout in httpd (currently 30s)
static int tcpCacheConnTimeout = 3000; // ms

// ApiConnect
static char const * confLogin = NULL;
static char const * confPassword = NULL;
static char const * confApiHost = NULL;
static char const * confDeviceUuid = NULL;
static char const * confChildUuid = NULL;

static ci_conf_entry configVariables[] = {
	// LoggingDevice
	{"LogLevel", &confLogLevel, ci_cfg_set_str, "log level"},
	{"LogFile", &confLogFile, ci_cfg_set_str, "log file path"},
	// TcpCache
	{"WsUrlAddress", &wsUrlAddress, ci_cfg_set_str, "WS URL host"},
	{"WsUrlPort", &wsUrlPort, ci_cfg_set_int, "WS URL tcp port"},
	{"TcpCacheInactivityTime", &tcpCacheInactivityTime, ci_cfg_set_int, "max inactivity time of a connection to WS URL"},
	{"TcpCacheConnTimeout", &tcpCacheConnTimeout, ci_cfg_set_int, "timeout for connecting to WS URL"},
	// ApiConnect
	{"Login", &confLogin, ci_cfg_set_str, "SafeKiddo account login"},
	{"Password", &confPassword, ci_cfg_set_str, "SafeKiddo account password"},
	{"ApiHost", &confApiHost, ci_cfg_set_str, "API host"},
	{"DeviceUuid", &confDeviceUuid, ci_cfg_set_str, "device uuid"},
	{"ChildUuid", &confChildUuid, ci_cfg_set_str, "child uuid"},
	{NULL, NULL, NULL, NULL}
};

CI_DECLARE_MOD_DATA ci_service_module_t service = {
	"safekiddo",                         /* mod_name, The module name */
	"Safekiddo service",                 /* mod_short_descr,  Module short description */
	ICAP_REQMOD,                         /* mod_type, The service type is responce or request modification */
	safekiddo_init_service,              /* mod_init_service. Service initialization */
	safekiddo_post_init_service,         /* mod_post_init_service. Service initialization after c-icap configured */
	safekiddo_close_service,             /* mod_close_service. Called when service shutdowns. */
	safekiddo_init_request_data,         /* mod_init_request_data */
	safekiddo_release_request_data,      /* mod_release_request_data */
	safekiddo_check_preview_handler,     /* mod_check_preview_handler */
	safekiddo_end_of_data_handler,       /* mod_end_of_data_handler */
	safekiddo_io,                        /* mod_service_io */
	configVariables,                     /* mod_conf_table */
	NULL                                 /* mod_data */
};


/* This function will be called when the service loaded  */
int safekiddo_init_service(ci_service_xdata_t * srv_xdata, struct ci_server_conf *server_conf)
{
	ci_service_set_preview(srv_xdata, 0);
	ci_service_enable_204(srv_xdata);
	ci_service_set_xopts(srv_xdata, CI_XAUTHENTICATEDUSER);
	return CI_OK;
}

static boost::optional<su::LogLevelEnum> logLevelEnumOpt;
static boost::scoped_ptr<utils::TcpCache> tcpCache;
static boost::scoped_ptr<utils::ApiConnect> apiConnect;

static std::string tryGetChildUuid()
{
	std::string childUuid = apiConnect->getChildUuid();
	if (childUuid.empty())
	{
		std::string cookie; // unused
		if (apiConnect->accessBlockedPage(cookie))
		{
			childUuid = apiConnect->getChildUuid();
		}
		else
		{
			LOG(WARNING, "Cannot access block page");
		}
		if (childUuid.empty())
		{
			LOG(ERROR, "Cannot obtain child uuid");
		}
	}
	return childUuid;
}

/*
 * Called just after forking a new child process of c-icap server. Requests are handled by child
 * processes. Parent process acts as a supervisor.
 */
static void fork_handler_child()
{
	ci_debug_printf(2, "creating LoggingDevice\n");

	std::string const logFilePath = std::string(confLogFile);
	su::LoggingDevice * loggingDevice = new su::FileLoggingDevice(logFilePath, logLevelEnumOpt.get());

	DASSERT(su::LoggingDevice::getImpl() == NULL, "LoggingDevice already set");
	su::LoggingDevice::setImpl(loggingDevice);
	loggingDevice->start();

	std::string const childUuid = tryGetChildUuid();
	if (!childUuid.empty())
	{
		LOG(INFO, "Starting with childUuid: " << childUuid);
	}
	else
	{
		// Crash in debug config if child uuid is not known.
		ALERT("Cannot obtain child uuid");
	}
}

static bool verifyStringParams()
{
	bool ret = true;
	for (size_t i = 0; i < sizeof(configVariables)/sizeof(configVariables[0]); ++i)
	{
		ci_conf_entry const & confEntry = configVariables[i];
		if (confEntry.action == ci_cfg_set_str)
		{
			char const * value = *static_cast<char const **>(confEntry.data);
			if (value == NULL)
			{
				std::cerr << "Error: missing safekiddo." << confEntry.name << " parameter\n";
				ret = false;
			}
		}
	}
	return ret;
}

// Don't use LOG here as LoggingDevice is set in child processes.
int safekiddo_post_init_service(ci_service_xdata_t * srv_xdata, struct ci_server_conf *server_conf)
{
	if (!verifyStringParams())
	{
		return CI_ERROR;
	}

	// LoggingDevice
	logLevelEnumOpt = su::LoggingDevice::parseLogLevel(confLogLevel);
	if (!logLevelEnumOpt)
	{
		std::cerr << "Error: invalid safekiddo.LogLevel argument: " << confLogLevel << "\n";
		return CI_ERROR;
	}

	// TcpCache
	// Constructor does not create new threads nor connections so is fork-safe.
	tcpCache.reset(new utils::TcpCache(wsUrlAddress, wsUrlPort, tcpCacheInactivityTime, tcpCacheConnTimeout));

	// ApiConnect
	// Constructor does not create new threads nor connections so is fork-safe.
	apiConnect.reset(new utils::ApiConnect(
		confLogin,
		confPassword,
		confApiHost,
		"/api/v1/",
		"login",
		"childs",
		"child/set",
		"block",
		confDeviceUuid,
		confChildUuid
	));

	ci_debug_printf(2, "installing fork handler\n");
	pthread_atfork(NULL, NULL, fork_handler_child);
	// Parent will not have LoggingDevice, but it is really needed only in children, which handle client requests.

	return CI_OK;
}

/* This function will be called when the service shutdown */
void safekiddo_close_service()
{
	apiConnect.reset();
	tcpCache.reset();

	su::LoggingDevice * loggingDevice = su::LoggingDevice::getImpl();
	if (loggingDevice)
	{
		ci_debug_printf(2, "destroying LoggingDevice\n");
		loggingDevice->stop();
		su::LoggingDevice::setImpl(NULL);
		delete loggingDevice;
	}
}


void printer(void*, const char *key, const char *value)
{
	DLOG(1, "HDR : " << key << ", VAL : " << value);
}

void safekiddo_print_request_headers(ci_request_t *req)
{
	ci_headers_iterate(req->request_header, NULL, printer);
}

const char *safekiddo_get_authenticated_user(ci_request_t *req)
{
//	safekiddo_print_request_headers(req);
	const char *key = "X-Authenticated-User: ";
	const char *hdr = ci_headers_search(req->request_header, key);
	if (!hdr)
	{
		return NULL;
	}
	return hdr + strlen(key);
}

struct body_data_t
{
	ci_ring_buf_t *body;
	int eof;
};

void *safekiddo_init_request_data(ci_request_t * req)
{
	struct body_data_t *body_data;

	/*Allocate memory fot the echo_data*/
	body_data = (body_data_t*) malloc(sizeof(struct body_data_t));
	if (!body_data)
	{
		ci_debug_printf(1, "Memory allocation failed inside safekiddo_init_request_data!\n");
		return NULL;
	}

	/*If the ICAP request encuspulates a HTTP objects which contains body data
	and not only headers allocate a ci_cached_file_t object to store the body data.
	*/
	if (ci_req_hasbody(req))
	{
		body_data->body = ci_ring_buf_new(4096);
	}
	else
	{
		body_data->body = NULL;
	}

	body_data->eof = 0;
	/*Return to the c-icap server the allocated data*/
	return body_data;
}

void safekiddo_release_request_data(void *data)
{
	struct body_data_t *body_data = (struct body_data_t *) data;

	if(body_data->body)
	{
		ci_ring_buf_destroy(body_data->body);
	}

	free(body_data);
}

/**
 * Function splits request in form: METHOD URL PROTOCOL (e.g. GET www.onet.pl HTTP/1.1) to
 * separate (sub)strings.
 */

static int splitRequest(
		const std::string &request, // original request string
		std::string &method, // will set to method name
		std::string &url, // will set to url
		std::string &proto) // will set to protocol string
{
	const size_t start = request.find(' ');
	if (start == std::string::npos)
	{
		return -1; // cannot find valid url start
	}

	const size_t end = request.rfind(' ');
	assert(end != std::string::npos);

	if (end == start || end == request.length() - 1) // end should not be the last element
	{
		return -1; // cannot find valud url end
	}
	// convert tail to lower case - that is why we have to check if 'end' is not at the end of the string
	proto = request.substr(end + 1);

	const std::string httpEnd = "http/1.";
	if (!su::isPrefixOfNoCase(httpEnd, proto))
	{
		return -1; // invalid request tail
	}

	method = request.substr(0, start);
	url = request.substr(start + 1, end - start - 1);

	return 0;
}

static inline bool contains(std::string const & str, char const * text)
{
	return str.find(text) != std::string::npos;
}

static std::vector<std::string> filterOutParamKeys(std::vector<std::string> const & params, char const * key)
{
	std::vector<std::string> result;

	FOREACH_CREF(param, params)
	{
		std::string decoded;
		su::percentDecode(param.substr(0, param.find('=')), decoded);
		if (decoded != key)
		{
			result.push_back(param);
		}
	}

	return result;
}

static boost::optional<std::string> modifyGoogleSearchUrl(std::string const & url)
{
	std::string protocolSpecification;
	std::string hostNameWithOptPort;
	std::string pathAndQueryString;
	std::string fragmentId;
	su::splitUrl(url, protocolSpecification, hostNameWithOptPort, pathAndQueryString, fragmentId);

	std::transform(hostNameWithOptPort.begin(), hostNameWithOptPort.end(), hostNameWithOptPort.begin(), ::tolower);
	if (!contains(hostNameWithOptPort, "google."))
	{
		return boost::none;
	}

	std::string path;
	std::vector<std::string> params;
	su::splitPathAndQueryString(pathAndQueryString, path, params);

	std::string decoded;
	su::percentDecode(path, decoded);
	// only lowercase works
	if (!contains(decoded, "/search"))
	{
		return boost::none;
	}

	char const * safeSearchKey = "safe";
	char const * safeSearchValue = "active";

	params = filterOutParamKeys(params, safeSearchKey);
	params.push_back(std::string(safeSearchKey) + "=" + safeSearchValue);

	return su::joinUrlParts(protocolSpecification, hostNameWithOptPort, path, params, fragmentId);
}

int writeWsRequest(boost::shared_ptr<utils::TcpSocket> sock,  std::string const & data, int timeout)
{
	char const * buf = data.c_str();
	size_t len = data.length();

	while (len)
	{
		ssize_t const wr = sock->write(buf, len);
		if (wr > 0)
		{
			len -= wr;
			buf += wr;
		}
		else
		{
			switch (wr)
			{
			case -EAGAIN:
			{
				int const res = sock->waitWrite(timeout);
				if (res == 1)
				{
					continue;
				}
				if (res == 0)
				{
					LOG(ERROR, "Timeout in write(): " << utils::sysError());
					return -1;
				}
				// fall through
			}
			default:
				LOG(ERROR, "Error in write(): " << utils::sysError());
				return -1;
			}
		}
	}

	return 0;
}

int readWsResponse(boost::shared_ptr<utils::TcpSocket> sock, int timeout)
{
	const size_t bufSize = 1023;
	char buf[bufSize + 1]; // we terminate with '\0'
	size_t inBuf = 0;
	buf[inBuf] = 0;

	while (true)
	{
		ssize_t const rd = sock->read(buf + inBuf, bufSize - inBuf);
		if (rd > 0)
		{
			inBuf += rd;
			buf[inBuf] = 0;
		}
		else
		{
			switch (rd)
			{
			case 0:
				LOG(ERROR, "WS URL aborted connection, data already received: '" << buf << "'");
				return -1;
			case -EAGAIN:
			{
				int const res = sock->waitRead(timeout);
				if (res == 1)
				{
					continue;
				}
				if (res == 0)
				{
					LOG(ERROR, "Timeout in read(): " << utils::sysError());
					return -1;
				}
				// fall through
			}
			default:
				LOG(ERROR, "Error in read(): " << utils::sysError());
				return -1;
			}
		}

		if (strstr(buf, "\nhack\n"))
		{
			break;
		}
	}

	char const * resultPrefix = "\nResult: ";

	char const * resultStart = strstr(buf, resultPrefix);
	if (resultStart == NULL)
	{
		LOG(ERROR, "No result in WS response", buf);
		return -1;
	}
	std::istringstream istr(resultStart + strlen(resultPrefix));
	int result;
	istr >> result;
	return result;
}


int wsRequest(const std::string &url, const std::string &user)
{
	if (user.empty())
	{
		LOG(WARNING, "User not known, not sending WS URL request", url);
		return 0;
	}

	DLOG(1, "Requesting ws: " << url);
	int const wsTimeout = 3000;
	bool const logging = false;
	std::stringstream sStream;

	std::string userAction = "";
	if(!logging)
	{
		userAction = "UserAction: 0\r\n";
	}

	// keep-alive by default in HTTP/1.1
	sStream << "GET /cc HTTP/1.1\r\nUserId: " << user << "\r\nRequest:" << url << "\r\nHost: " << tcpCache->getHostName() << "\r\n"
		<< userAction << "\r\n";

	boost::shared_ptr<utils::TcpSocket> socket = tcpCache->connect();
	if (!socket)
	{
		return -1;
	}

	if (writeWsRequest(socket, sStream.str(), wsTimeout))
	{
		return -1;
	}

	int const ret = readWsResponse(socket, wsTimeout);
	if (ret < 0)
	{
		LOG(ERROR, "Error in ws communication");
		return -1;
	}
	else
	{
		tcpCache->add(socket);
		return ret;
	}
}

int safekiddo_check_preview_handler(char * preview_data, int preview_data_len, ci_request_t * req)
{
	char const * authUser = safekiddo_get_authenticated_user(req);
	std::string const user = authUser ? std::string(authUser) : tryGetChildUuid();

	std::string url, method, proto;

	if (splitRequest(ci_http_request(req), method, url, proto))
	{
		LOG(ERROR, "Failed to parse http request '" << ci_http_request(req) << "'");
		return CI_MOD_ALLOW204;
	}
	if (strcasecmp(method.c_str(), "CONNECT") == 0)
	{
		DLOG(1, "Not altering CONNECT to '" << url << "'");
		return CI_MOD_ALLOW204;
	}

	DLOG(1, "User '" << user << "' requested url '" << url << "' method '" << method << "' proto '" << proto << "'");

	boost::optional<std::string> const modifiedUrl = modifyGoogleSearchUrl(url);
	int const wsUrlResult = wsRequest(url, user);

	bool const isSslRequest = su::isPrefixOfNoCase("https://", url);
	// Workaround for SAF-515: when using ssl-bump, ICAP request modification supports only HTTP urls. If different HTTPS url
	// is provided then no new connection is established by ssl-bump and we end up with a 404 from original target host.
	std::string const blockPageUrl = isSslRequest ? apiConnect->getBlockedPageUrlNoSsl() : apiConnect->getBlockedPageUrl();
	bool const isApiUrl = (url == blockPageUrl);

	if (modifiedUrl)
	{
		DLOG(1, "url modified: '" << url << "' => '" << modifiedUrl.get() << "'");
	}
	/* TODO: SAF-508 - parent requests don't work
	else if (isApiUrl)
	{
		DLOG(1, "request to a block page", url);
	}*/
	else if (wsUrlResult > 0)
	{
		LOG(INFO, "Access denied for user '" << user << "' result " << wsUrlResult << " url '" << url << "'");
	}
	else
	{
		return CI_MOD_ALLOW204;
	}

	ci_headers_list_t *list = ci_http_request_headers(req);

	std::vector<std::string> headers;
	for (int i = 0; i < list->used && list->headers[i] != NULL; ++i)
	{
		headers.push_back(list->headers[i]);
	}

	ci_http_request_reset_headers(req);
	std::string const requestStartsWith = method + " ";
	std::string const cookieHeaderPrefix = "Cookie: ";
	std::string const hostHeaderPrefix = "Host: ";

	FOREACH_CREF(header, headers)
	{
		if (su::isPrefixOf(requestStartsWith, header))
		{
			std::ostringstream request;
			// URL was dangerous, but after modification is safe
			if (modifiedUrl)
			{
				request << method << " " << modifiedUrl.get() << " " << proto;
				ci_http_request_add_header(req, request.str().c_str());
				DLOG(1, "request changed to: " << request.str());
			}
			else // URL is still dangerous - we have to block this page
			{
				if(isApiUrl)
				{
					request << method << url << " " << proto;
				}
				else
				{
					// redirect to block page
					request << "GET " << blockPageUrl << " " << proto;
				}
				ci_http_request_add_header(req, request.str().c_str());
				DLOG(1, "request changed to: " << request.str());

				std::string cookie;
				if (!apiConnect->accessBlockedPage(cookie))
				{
					LOG(WARNING, "Cannot access block page");
				}
				else
				{
					std::stringstream cookieHeaderStream;
					cookieHeaderStream << cookieHeaderPrefix << cookie;
					ci_http_request_add_header(req, cookieHeaderStream.str().c_str());
				}

				if(!isApiUrl)
				{
					// add params needed by block page
					std::stringstream skUrlStream;
					skUrlStream << "sk-url: " << url;
					ci_http_request_add_header(req, skUrlStream.str().c_str());

					DASSERT(wsUrlResult > 0, "why do we block this url?", wsUrlResult, url);
					std::stringstream skCodeStream;
					skCodeStream << "sk-code: " << wsUrlResult;
					ci_http_request_add_header(req, skCodeStream.str().c_str());
				}
			}
		}
		else if(
			su::isPrefixOfNoCase(cookieHeaderPrefix, header) ||
			su::isPrefixOfNoCase(hostHeaderPrefix, header)
		)
		{
			DLOG(1, "skipping header '" << header << "'");
		}
		else
		{
			// add all remaining headers
			ci_http_request_add_header(req, header.c_str());
		}
	}
	return CI_MOD_CONTINUE;
}

int safekiddo_end_of_data_handler(ci_request_t * req)
{
	return CI_MOD_DONE;
}

int safekiddo_io(char *wbuf, int *wlen, char *rbuf, int *rlen, int iseof, ci_request_t * req)
{
	int ret;
	struct body_data_t *body_data = (body_data_t*) ci_service_data(req);
	ret = CI_OK;

	 /*write the data read from icap_client to the echo_data->body*/
	 if(rlen && rbuf) {
		 *rlen = ci_ring_buf_write(body_data->body, rbuf, *rlen);
		 if (*rlen < 0)
		ret = CI_ERROR;
	 }

	 /*read some data from the echo_data->body and put them to the write buffer to be send
	  to the ICAP client*/
	 if (wbuf && wlen) {
		  *wlen = ci_ring_buf_read(body_data->body, wbuf, *wlen);
	 }
	 if(*wlen==0 && body_data->eof==1)
	 *wlen = CI_EOF;

	 return ret;
}

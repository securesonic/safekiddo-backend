#include "apiConnect.h"
#include "utils/loggingDevice.h"
#include "utils/stringFunctions.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace su = safekiddo::utils;

namespace utils
{

ApiConnect::ApiConnect(
	std::string const & login,
	std::string const & password,
	std::string const & apiHost,
	std::string const & apiVersionPath,
	std::string const & apiLoginPath,
	std::string const & apiChildListPath,
	std::string const & apiSetChildPath,
	std::string const & apiBlockedPagePath,
	std::string const & deviceUuid,
	std::string const & childUuid
):
	login(login),
	password(password),
	apiHost(apiHost),
	apiVersionPath(apiVersionPath),
	apiLoginPath(apiLoginPath),
	apiChildListPath(apiChildListPath),
	apiSetChildPath(apiSetChildPath),
	apiBlockedPagePath(apiBlockedPagePath),
	deviceUuid(deviceUuid),
	childUuid(childUuid),
	httpSender(apiHost, true),
	cookie("")
{
}

std::string ApiConnect::getBlockedPageUrl() const
{
	return "https://" + this->apiHost + this->getFullPath(this->apiBlockedPagePath);
}

std::string ApiConnect::getBlockedPageUrlNoSsl() const
{
	return "http://" + this->apiHost + this->getFullPath(this->apiBlockedPagePath);
}

std::string ApiConnect::getChildUuid() const
{
	return this->childUuid;
}

/*
 * Sends a HTTP request. Sets cookie if successful.
 */
bool ApiConnect::accessBlockedPage(std::string & cookie)
{
	std::unique_lock<std::mutex> getBlockedLock(this->getBlockedPageMutex);
	if(this->canConnectToBlockPage() || this->resetSession())
	{
		cookie = this->cookie;
		return true;
	}
	return false;
}

bool ApiConnect::loginToSafekiddo()
{
	std::vector<std::string> otherHeaders;
	std::vector<std::string> responseHeaders;
	std::string responseBody;
	std::stringstream ss;
	ss << "username=" << this->login << "&password=" << this->password;
	std::string const apiPath = this->getFullPath(this->apiLoginPath);
	this->httpSender.sendRequest(apiPath, "POST", otherHeaders, ss.str(), responseHeaders, responseBody);

	if(this->handleResponseGeneric(responseBody, apiPath))
	{
		for(auto &header : responseHeaders)
		{
			if(su::isPrefixOfNoCase(this->setCookieHeaderPrefix, header))
			{
				this->cookie = header.substr(this->setCookieHeaderPrefix.length());
				return true;
			}
		}
	}
	return false;
}

bool ApiConnect::setChildRequest()
{
	std::vector<std::string> otherHeaders;
	otherHeaders.push_back(this->getUserAgentHeader());
	otherHeaders.push_back(this->getCookieHeader());
	std::stringstream requestBodyStream;
	requestBodyStream << "child_uuid=" << this->childUuid << "&";
	requestBodyStream << "device_uuid=" << this->deviceUuid << "&";
	requestBodyStream << "device_label=" << this->deviceLabel;
	std::vector<std::string> responseHeaders;
	std::string responseBody;
	std::string const apiPath = this->getFullPath(this->apiSetChildPath);
	this->httpSender.sendRequest(apiPath, "POST", otherHeaders, requestBodyStream.str(), responseHeaders, responseBody);
	return this->handleResponseGeneric(responseBody, apiPath);
}

bool ApiConnect::resetSession()
{
	return this->loginToSafekiddo() && this->setChildRequest();
}

std::string const ApiConnect::getFullPath(std::string const & path) const
{
	return this->apiVersionPath + path;
}

std::string const ApiConnect::getCookieHeader() const
{
	return this->cookieHeaderPrefix + this->cookie;
}

std::string const ApiConnect::getUserAgentHeader() const
{
	return this->userAgentHeaderPrefix + this->userAgent;
}

bool ApiConnect::canConnectToBlockPage()
{
	if (this->cookie.empty())
	{
		DLOG(1, "canConnectToBlockPage: false, cookie not set");
		return false;
	}
	std::vector<std::string> otherHeaders;
	otherHeaders.push_back(this->getCookieHeader());
	std::vector<std::string> responseHeaders;
	std::string responseBody;
	bool const ret = this->httpSender.sendRequest(
		this->getFullPath(this->apiBlockedPagePath),
		"GET",
		otherHeaders,
		"", // body
		responseHeaders,
		responseBody
	);
	DLOG(1, "canConnectToBlockPage: " << (ret ? "true" : "false"));
	return ret;
}

bool ApiConnect::handleResponseGeneric(std::string const & response, std::string const & apiPath)
{
	bool success = false;
	try
	{
		using boost::property_tree::ptree;
		ptree pt;
		std::stringstream responseStream(response);
		read_json(responseStream, pt);

		success = pt.get("success", false);

		if(!success)
		{
			LOG(ERROR, "handleResponseGeneric FAILED", apiPath, response);
		}
		else
		{
			LOG(INFO, "handleResponseGeneric SUCCEED", apiPath, response);
		}
	}
	catch(boost::property_tree::json_parser_error & e)
	{
		LOG(ERROR, "handleResponseGeneric error", apiPath, e.what(), response);
	}
	return success;
}

} // namespace utils

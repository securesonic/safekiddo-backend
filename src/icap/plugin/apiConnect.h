#ifndef API_CONNECT_H_
#define API_CONNECT_H_

#include <string>
#include <mutex>
#include "utils/httpSender.h"

namespace utils
{

class ApiConnect
{
public:
	ApiConnect(
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
	);

	std::string getBlockedPageUrl() const;
	std::string getBlockedPageUrlNoSsl() const;
	std::string getChildUuid() const;
	bool accessBlockedPage(std::string & cookie);

private:
	std::string const setCookieHeaderPrefix = "Set-Cookie: ";
	std::string const cookieHeaderPrefix = "Cookie: ";
	std::string const userAgentHeaderPrefix = "user-agent: ";

	std::string const login;
	std::string const password;

	std::string const apiHost;
	std::string const apiVersionPath;
	std::string const apiLoginPath;
	std::string const apiChildListPath;
	std::string const apiSetChildPath;
	std::string const apiBlockedPagePath;

	std::string const deviceUuid;
	std::string const childUuid;
	std::string const deviceLabel = "School Appliance";
	std::string const userAgent = "SafeKiddo Appliance (v1.0.0; http://www.safekiddo.com)";

	safekiddo::utils::HttpSender httpSender;
	std::mutex mutable getBlockedPageMutex;
	std::string cookie;

	bool loginToSafekiddo();
	bool getChildListRequest();
	bool setChildRequest();
	bool resetSession();
	bool canConnectToBlockPage();
	std::string const getFullPath(std::string const & path) const;
	std::string const getCookieHeader() const;
	std::string const getUserAgentHeader() const;

	bool handleResponseGeneric(std::string const & response, std::string const & apiPath);
	bool handleGetChildListResponse(std::string const & response);

};

} // namespace utils

#endif /* API_CONNECT_H_ */

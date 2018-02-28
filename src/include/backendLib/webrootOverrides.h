#ifndef _SAFEKIDDO_BACKEND_WEBROOT_OVERRIDES_H
#define _SAFEKIDDO_BACKEND_WEBROOT_OVERRIDES_H

#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <condition_variable>
#include <chrono>

#include <boost/thread/shared_mutex.hpp>

#include "config.h"
#include "dbClient.h"
#include "webrootCpp/categorization.h"

class c_md5;

namespace safekiddo
{
namespace backend
{

using namespace std::chrono;

class WebrootOverridesProvider
{
public:
	WebrootOverridesProvider(
		Config const & config,
		unsigned int updateInterval
	);
	~WebrootOverridesProvider();

	void forceUpdateMap();
	void start();
	void waitUntilReady();

	// returns a vector of categories
	webroot::Categorizations findCategorization(std::string const & url) const;

private:
	typedef std::map<c_md5, webroot::Categorization> CategorizationsMap;

	void updateLoop();

	static bool fetchOverrides(db::DBConnection const & dbConnection, CategorizationsMap & result);

	system_clock::time_point getNextUpdateTime() const;

	boost::shared_mutex mutable categorizationsMutex;
	// keeps just one category
	CategorizationsMap categorizationsMap;

	std::mutex mutable sleepMutex;
	std::thread updateThread;
	std::condition_variable mutable stopCondition;
	bool stopped;
	bool forceUpdateRequired;
	std::condition_variable mutable readyCondition;
	bool ready; // true if db already downloaded

	Config const config;
	unsigned int updateInterval;
};

} // namespace backend
} // namespace safekiddo

#endif // _SAFEKIDDO_BACKEND_WEBROOT_OVERRIDES_H

#include "backendLib/webrootOverrides.h"

#include "utils/alerts.h"
#include "utils/urlNormalization.h"

#include "md5obj.h"
#include "lcpurl.h"
#include "sdkcfg.h"

#include <utility>
#include <algorithm>
#include <ctype.h>

namespace su = safekiddo::utils;

namespace safekiddo
{
namespace backend
{

WebrootOverridesProvider::WebrootOverridesProvider(
	Config const & config,
	unsigned int updateInterval
):
	categorizationsMutex(),
	categorizationsMap(),
	sleepMutex(),
	updateThread(),
	stopCondition(),
	stopped(false),
	forceUpdateRequired(false),
	readyCondition(),
	ready(false),
	config(config),
	updateInterval(updateInterval)
{
}

WebrootOverridesProvider::~WebrootOverridesProvider()
{
	{
		std::lock_guard<std::mutex> lock(this->sleepMutex);
		this->stopped = true;
		this->stopCondition.notify_one();
	}
	this->updateThread.join();
}

void WebrootOverridesProvider::forceUpdateMap()
{
	std::lock_guard<std::mutex> lock(this->sleepMutex);
	this->forceUpdateRequired = true;
	this->stopCondition.notify_one();
}

void WebrootOverridesProvider::start()
{
	this->updateThread = std::thread(&WebrootOverridesProvider::updateLoop, this);
}

void WebrootOverridesProvider::waitUntilReady()
{
	std::unique_lock<std::mutex> uniqueSleepLock(this->sleepMutex);
	this->readyCondition.wait(uniqueSleepLock, [this](){return this->ready;});
}

webroot::Categorizations WebrootOverridesProvider::findCategorization(std::string const & url) const
{
	DASSERT(this->ready, "db not yet loaded");
	webroot::Categorizations result;

	int nLcpFound = 0;
	std::string urlForWebroot = url;
	// [BUG id=SAF-449 fixer=sachanowicz date=2014-09-19 summary=lcp parser does not lower case of url]
	std::transform(urlForWebroot.begin(), urlForWebroot.end(), urlForWebroot.begin(), ::tolower);
	// webroot modifies input string; don't use c_str() here as it does not unshare the string
	cLcpParse lcpUrl(&urlForWebroot[0], psCfg->nLcpLoopLimit, nLcpFound);

	if (!nLcpFound)
	{
		DLOG(1, "can't lcp parse URL " << url);
		return result;
	}

	boost::shared_lock<boost::shared_mutex> lock(this->categorizationsMutex);

	c_md5 md5;
	char lcpBuffer[URL_SIZE];
	while (lcpUrl.nGetLcp(&urlForWebroot[0], lcpBuffer))
	{
		DLOG(3, "findCategorization: url: " << url << " next lcp: " << lcpBuffer);
		md5.setByUrl(lcpBuffer);
		CategorizationsMap::const_iterator it = this->categorizationsMap.find(md5);
		if (it != this->categorizationsMap.end())
		{
			DLOG(1, "using webroot override: " << lcpBuffer << " -> " << it->second, url);
			result.push_back(it->second);
			break;
		}
	}
	return result;
}

static std::vector<db::PreparedStatement> const preparedStatements =
{
	{
		"get_overrides",
		"select address, code_categories_ext_id from webroot_overrides",
		0
	}
};

void WebrootOverridesProvider::updateLoop()
{
	LOG(INFO, "webroot overrides update thread started");

	db::DBConnection::ResultType connectionResult = db::DBConnection::create(
		this->config.dbHost,
		this->config.dbPort,
		this->config.dbName,
		this->config.dbUser,
		this->config.dbPassword,
		this->config.sslMode
	);

	if (connectionResult.isError())
	{
		LOG(ERROR, "unable to connect to db: " << connectionResult.getError());
		FAILURE("Cannot connect to database.");
	}

	db::DBConnection & connection = *connectionResult.getValue();
	connection.setSocketTimeout(boost::posix_time::seconds(30));
	connection.setPreparedStatements(preparedStatements);

	while (true)
	{
		CategorizationsMap newCategorizationsMap;
		if (this->fetchOverrides(connection, newCategorizationsMap))
		{
			boost::unique_lock<boost::shared_mutex> lock(this->categorizationsMutex);
			this->categorizationsMap = std::move(newCategorizationsMap);
		}
		else
		{
			LOG(ERROR, "cannot fetch webroot overrides from db");
			ASSERT(this->ready, "failed on first try, something must be wrong");
		}

		std::unique_lock<std::mutex> uniqueSleepLock(this->sleepMutex);
		if (!this->ready)
		{
			this->ready = true;
			this->readyCondition.notify_all();
		}
		if (this->stopCondition.wait_until(
			uniqueSleepLock,
			this->getNextUpdateTime(),
			[this](){return this->forceUpdateRequired || this->stopped;}
		))
		{
			if (this->stopped)
			{
				break;
			}
			if (this->forceUpdateRequired)
			{
				this->forceUpdateRequired = false;
			}
		}
	}
	LOG(INFO, "webroot overrides update thread finished");
}

system_clock::time_point WebrootOverridesProvider::getNextUpdateTime() const
{
	time_t now = system_clock::to_time_t(system_clock::now());
	time_t next = (now / this->updateInterval + 1) * this->updateInterval;
	return system_clock::from_time_t(next);
}

bool WebrootOverridesProvider::fetchOverrides(db::DBConnection const & dbConnection, CategorizationsMap & result)
{
	LOG(INFO, "fetching webroot overrides from db");

	db::DBConnection::QueryResult queryResult = dbConnection.queryPrepared(
		"get_overrides",
		0, // nParams
		NULL, // paramValues
		NULL, // paramLengths
		NULL // paramFormats
	);

	if (queryResult.isError())
	{
		return false;
	}

	std::list<db::DBConnection::row_t> const & rows = *queryResult.getValue();

	c_md5 md5;
	char lcpBuffer[URL_SIZE];

	for (auto const & row : rows)
	{
		std::string const & url = su::shortenUrl(row.at(0));
		webroot::Category const category = static_cast<webroot::Category>(boost::lexical_cast<unsigned int>(row.at(1)));

		int nLcpFound = 0;
		std::string urlForWebroot = url;
		std::transform(urlForWebroot.begin(), urlForWebroot.end(), urlForWebroot.begin(), ::tolower);
		// webroot modifies input string; don't use c_str() here as it does not unshare the string
		cLcpParse lcpUrl(&urlForWebroot[0], psCfg->nLcpLoopLimit, nLcpFound);

		if (!nLcpFound)
		{
			LOG(ERROR, "can't lcp parse URL from webroot overrides: " << url);
		}
		else
		{
			nLcpFound = lcpUrl.nGetLcp(&urlForWebroot[0], lcpBuffer);
			DASSERT(nLcpFound == 1, "not found?", url);
			md5.setByUrl(lcpBuffer);
			DLOG(1, "adding webroot override: " << lcpBuffer << " -> " << webroot::Categorization::forCategory(category), url);
			auto emplaceResult = result.emplace(md5, webroot::Categorization::forCategory(category));
			if (!emplaceResult.second)
			{
				ALERT("webroot override for url [first lcp: " << lcpBuffer << " original: " << url << "] already added");
			}
		}
	}

	return true;
}

} // namespace backend
} // namespace safekiddo

#include <vector>
#include <string>
#include <fstream>
#include <boost/thread/thread.hpp>

#include "utils/loggingDevice.h"
#include "utils/foreach.h"
#include "webrootCpp/config.h"
#include "webrootCpp/urlInfo.h"
#include "webrootCpp/api.h"

namespace safekiddo
{
namespace backend
{
namespace testing
{

using namespace webroot;
using namespace std;

enum
{
	NUM_URLS = 100,
	NUM_THREADS = 10,
	BCAP_CACHE_TTL_SEC = 5
};

vector<string> readUrls()
{
	vector<string> result(NUM_URLS);
	ifstream urlFile("../../scripts/tests/alexa100k.urls.txt");

	for (size_t i = 0; i < NUM_URLS; i++)
	{
		urlFile >> result.at(i);
	}
	return result;
}

void workerFunction(uint32_t const threadIdx, vector<string> const & urls)
{
	int socket = 0;
	uint32_t const urlsPerThread = NUM_URLS / NUM_THREADS;
	uint32_t urlBaseIdx = threadIdx * urlsPerThread;
	for (uint32_t urlOffset = 0; urlOffset < urlsPerThread; urlOffset++)
	{
		string const url = urls.at(urlBaseIdx + urlOffset);
		UrlInfo const urlInfo = webroot::BcLookup(url, socket);
		LOG(INFO, "classification of " << url << ", got: " << urlInfo);
		ASSERT(urlInfo.getClassificationSource() == UrlInfo::BCAP, "expected reply from cloud!", url, urlInfo);
	}
	LOG(INFO, "checking presence of urls in BCAP_CACHE on the second run");
	for (uint32_t urlOffset = 0; urlOffset < urlsPerThread; urlOffset++)
	{
		string const url = urls.at(urlBaseIdx + urlOffset);
		UrlInfo const urlInfo = webroot::BcLookup(url, socket);
		LOG(INFO, "classification of " << url << ", got: " << urlInfo);
		ASSERT(urlInfo.getClassificationSource() == UrlInfo::BCAP_CACHE, "expected reply from cloud cache!", url, urlInfo);
	}
	sleep(BCAP_CACHE_TTL_SEC * 2);
	LOG(INFO, "checking invalidation of urls in BCAP_CACHE on the third run");
	for (uint32_t urlOffset = 0; urlOffset < urlsPerThread; urlOffset++)
	{
		string const url = urls.at(urlBaseIdx + urlOffset);
		UrlInfo const urlInfo = webroot::BcLookup(url, socket);
		LOG(INFO, "classification of " << url << ", got: " << urlInfo);
		ASSERT(urlInfo.getClassificationSource() == UrlInfo::BCAP, "expected reply from cloud!", url, urlInfo);
	}
}

void webrootCloudCacheTest()
{
	WebrootConfig webrootConfig = webroot::BcReadConfig("bcsdk.cfg");

	webrootConfig
		.disableDbDownload()
		.disableLocalDb()
		.disableRtu()
		.enableBcapCache()
		.enableBcap()
		.setBcapCacheTtlSec(BCAP_CACHE_TTL_SEC)
		.setOem("ardura")
		.setUid("ardura_dmz1223p98")
		.setDevice("prod_ardura_1");

	BcInitialize(webrootConfig);
	BcWaitForDbLoadComplete(webrootConfig);

	vector<string> urls = readUrls();

	boost::thread_group workerThreads;
	for (uint32_t threadIdx = 0; threadIdx < NUM_THREADS; threadIdx++)
	{
		workerThreads.create_thread(boost::bind(&workerFunction, threadIdx, boost::cref(urls)));
	}
	// Wait for threads to finish
	workerThreads.join_all();

	BcShutdown();
}

} // namespace testing
} // namespace backend
} // namespace safekiddo

int32_t main()
{
	std::ofstream logFile;
	logFile.exceptions(std::ios::failbit | std::ios::badbit);
	logFile.open("webrootCloudCacheTest.log", std::ios::app);
	safekiddo::utils::StdLoggingDevice loggingDevice(safekiddo::utils::LogLevel::DEBUG1, logFile);
	safekiddo::utils::ScopedLoggingDeviceSetter logSetter(&loggingDevice);

	safekiddo::backend::testing::webrootCloudCacheTest();
	return 0;
}


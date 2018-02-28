/*
 * TEST SCENARIOS:
 *
 * webrootDbUpdatesTest:
 * test for verifying db update code
 * 		load default db;
 * 		check version
 * 		turn on updates
 * 		wait for update
 * 		verify version is updated
 *
 */


#include <vector>
#include <string>
#include <fstream>
#include <boost/thread/thread.hpp>
#include <unistd.h>

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
	NUM_THREADS = 10
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
		ASSERT(urlInfo.getClassificationSource() == UrlInfo::BCAP, "expected reply from cloud!");
	}
	LOG(INFO, "checking presence of urls in BCAP_CACHE on the second run");
	for (uint32_t urlOffset = 0; urlOffset < urlsPerThread; urlOffset++)
	{
		string const url = urls.at(urlBaseIdx + urlOffset);
		UrlInfo const urlInfo = webroot::BcLookup(url, socket);
		LOG(INFO, "classification of " << url << ", got: " << urlInfo);
		ASSERT(urlInfo.getClassificationSource() == UrlInfo::BCAP_CACHE, "expected reply from cloud cache!");
	}
}

bool isDbNewer(sInternalStatus const & first, sInternalStatus const & second)
{
	return
		first.nDbVersionMajor > second.nDbVersionMajor ||
		first.nDbVersionMinor > second.nDbVersionMinor;
}

void webrootDbUpdatesTest()
{
	WebrootConfig webrootConfig = webroot::BcReadConfig("bcsdk.cfg");

	webrootConfig
		.enableDbDownload()
		.setRtuIntervalMin(1)
		.enableRtu()
		.setOem("ardura")
		.setUid("ardura_dmz1223p98")
		.setDevice("prod_ardura_1");

	webroot::BcSetConfig(webrootConfig);
	psSdkStatus->bSdkMode = false;

	webroot::BcSdkInit();

	webroot::BcLogConfig(webrootConfig);

	webroot::BcStartRtu();
	webroot::BcWaitForDbLoadComplete(webrootConfig);

	// FIXME: possible race condition. We assume here that db version is still
	// as for the initially loaded db (bcdb.bin).
	sInternalStatus const initialStatus = *psSdkStatus;
	LOG(INFO, "initial db version: ", initialStatus.nDbVersionMajor, initialStatus.nDbVersionMinor, initialStatus.nRtuVersion);

	enum
	{
		RETRIES = 40
	};

	bool dbUpdated = false;
	for(int32_t retry = 0; retry < RETRIES; retry++)
	{
		LOG(INFO, "current db version: ", psSdkStatus->nDbVersionMajor, psSdkStatus->nDbVersionMinor, psSdkStatus->nRtuVersion);
		if (isDbNewer(*psSdkStatus, initialStatus) && psSdkStatus->nRtuVersion > initialStatus.nRtuVersion)
		{
			dbUpdated = true;
			break;
		}
		sleep(5);
	}

	ASSERT(dbUpdated, "db not updated?");

	// Previously it just waited until db update is done. Currently it aborts a download.
	webroot::BcStopRtu();
}

} // namespace testing
} // namespace backend
} // namespace safekiddo

int32_t main()
{
	std::ofstream logFile;
	logFile.exceptions(std::ios::failbit | std::ios::badbit);
	logFile.open("webrootDbUpdatesTest.log", std::ios::app);
	logFile.setf(std::ios::unitbuf);
	safekiddo::utils::StdLoggingDevice loggingDevice(safekiddo::utils::LogLevel::DEBUG1, logFile);
	safekiddo::utils::ScopedLoggingDeviceSetter logSetter(&loggingDevice);

	safekiddo::backend::testing::webrootDbUpdatesTest();
	return 0;
}


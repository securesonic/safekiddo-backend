/*
 * TEST SCENARIOS:
 *
 * webrootDbRemovalTest:
 * test for removing old dbs from disk
 * 		create fake db files
 * 		run garbage collecting function
 * 		check only expected file (newest database) remained
 *
 */

#include <vector>
#include <string>
#include <fstream>
#include <boost/thread/thread.hpp>
#include <string.h>

#include "utils/loggingDevice.h"
#include "utils/foreach.h"
#include "webrootCpp/config.h"
#include "webrootCpp/urlInfo.h"
#include "webrootCpp/api.h"

#include "utils/containerPrinter.h"

using safekiddo::utils::makeContainerPrinter;

// webroot functions
int getDirList(std::string dir, std::vector<std::string> &files);
void clearOldDatabases(std::string const dbDirectory, std::string const currentDbFileName);

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

void issueSystemCall(std::string const command)
{
	int const result = system(command.c_str());
	ASSERT(result != -1, "command failed: ", command, result);
}

void webrootDbRemovalTest()
{
	string const dbDir("testdir12345");
	string const validDbName("full_bcdb_rep_4.301.bin");

	issueSystemCall("rm -rf " + dbDir);
	issueSystemCall("mkdir " + dbDir);

	string const fullDbPrefix("full_bcdb_");
	string const fullDbExtension(".bin");

	issueSystemCall("touch " + dbDir + "/full_bcdb_rep_4.300.bin");
	issueSystemCall("touch " + dbDir + "/" + validDbName);
	issueSystemCall("touch " + dbDir + "/full_bcdb_rep_4.302.zbin");
	issueSystemCall("touch " + dbDir + "/part_bcdb_rep_4.302.bin");

	WebrootConfig webrootConfig = webroot::BcReadConfig("bcsdk.cfg");

	webrootConfig
		.disableDbDownload()
		.disableRtu()
		.setDbDir("testdir12345");

	webroot::BcSetConfig(webrootConfig);
	webroot::BcSdkInit();
	webroot::BcLogConfig(webrootConfig);

	{
		vector<string> files;
		getDirList(dbDir, files);
		// 6 == 4 files + 2 artificial entries: . and ..
		ASSERT(files.size() == 6, "unexpected files: ", makeContainerPrinter(files));
	}

	LOG(INFO, "dbdir: ", psCfg->szLocalDatabaseDir);
	strcpy(psSdkStatus->szDbFileName, "full_bcdb_rep_4.301.bin");

	clearOldDatabases(psCfg->szLocalDatabaseDir, psSdkStatus->szDbFileName);

	{
		vector<string> files;
		getDirList(dbDir, files);
		// 3 == 1 file + 2 artificial entries: . and ..
		ASSERT(files.size() == 3, "unexpected files: ", makeContainerPrinter(files));
		bool matchFound = false;
		for (unsigned i = 0; i < files.size(); i++)
		{
			if (files[i] == validDbName)
			{
				matchFound = true;
				break;
			}
		}
		ASSERT(matchFound, "unexpected files", makeContainerPrinter(files));
	}

	issueSystemCall("rm -rf " + dbDir);
}

} // namespace testing
} // namespace backend
} // namespace safekiddo

int32_t main()
{
	std::ofstream logFile;
	logFile.exceptions(std::ios::failbit | std::ios::badbit);
	logFile.open("webrootDbRemovalTest.log", std::ios::app);
	logFile.setf(std::ios::unitbuf);
	safekiddo::utils::StdLoggingDevice loggingDevice(safekiddo::utils::LogLevel::DEBUG1, logFile);
	safekiddo::utils::ScopedLoggingDeviceSetter logSetter(&loggingDevice);

	safekiddo::backend::testing::webrootDbRemovalTest();
	return 0;
}


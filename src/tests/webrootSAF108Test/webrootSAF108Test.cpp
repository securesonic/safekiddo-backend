/*
 * TEST SCENARIOS:
 *
 * webrootSAF108Test:
 * test verifying fix for SIGSEGV if sConnectSocket returns 0 (failure) for local db update
 * 		load default db;
 * 		check version
 * 		turn on updates
 * 		test
 */


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

void webrootSAF108Test()
{
	WebrootConfig webrootConfig = webroot::BcReadConfig("bcsdk.cfg");

	webrootConfig
		.enableDbDownload()
		.setRtuIntervalMin(1)
		.enableRtu()
		.setBcapServer("bcap.invalid.server");

	webroot::BcSetConfig(webrootConfig);
	psSdkStatus->bSdkMode = false; // pretend we are not evaluating

	webroot::BcSdkInit();

	webroot::BcLogConfig(webrootConfig);

	sInternalStatus const initialStatus = *psSdkStatus;
	LOG(INFO, "initial db version: ", initialStatus.nDbVersionMajor, initialStatus.nDbVersionMinor, initialStatus.nRtuVersion);

	webroot::BcStartRtu();
	webroot::BcWaitForDbLoadComplete(webrootConfig);
	// I dont have a good idea how to check whether the actual bug scenario fires in the test
	sleep(5);
	webroot::BcStopRtu();
	LOG(INFO, "webroot SDK did not segfault; lets hope for the best;");
}


} // namespace testing
} // namespace backend
} // namespace safekiddo

int32_t main()
{
	std::ofstream logFile;
	logFile.exceptions(std::ios::failbit | std::ios::badbit);
	logFile.open("webrootSAF108Test.log", std::ios::app);
	logFile.setf(std::ios::unitbuf);
	safekiddo::utils::StdLoggingDevice loggingDevice(safekiddo::utils::LogLevel::DEBUG1, logFile);
	safekiddo::utils::ScopedLoggingDeviceSetter logSetter(&loggingDevice);

	safekiddo::backend::testing::webrootSAF108Test();
	return 0;
}


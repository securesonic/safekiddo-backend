#include <boost/program_options.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/scoped_ptr.hpp>

#include <stdint.h>
#include <fstream>
#include <iostream>

#include "backendLib/config.h"
#include "backendLib/requestLogger.h"
#include "backendLib/currentTime.h"

#include "utils/loggingDevice.h"
#include "utils/foreach.h"

namespace sb = safekiddo::backend;
namespace su = safekiddo::utils;
namespace po = boost::program_options;

namespace safekiddo
{
namespace backend
{
namespace testing
{

class RequestLoggerTest
{
public:
	RequestLoggerTest(
		Config const & config,
		std::vector<std::string> & urls,
		uint32_t numIterations
	);

	void run();

private:
	void workerFunction(uint32_t threadIdx);

	uint32_t getTotalRequestsNum() const;

	void writeResult(double result);

	Config config;
	std::vector<std::string> const & urls;
	uint32_t numIterations;

	boost::scoped_ptr<RequestLogger> requestLogger;
};

void RequestLoggerTest::workerFunction(uint32_t threadIdx)
{
	LOG(INFO, "test thread started");
	std::ostringstream sStream;
	sStream << "RequestLoggerTest-" << threadIdx;
	for (uint32_t i = 0; i < this->numIterations; ++i)
	{
		FOREACH_CREF(url, this->urls)
		{
			BackendQueryResult queryResult;

			queryResult.utcTime = getCurrentUtcTime();
			queryResult.childTime = convertToLocalTime(queryResult.utcTime);

			queryResult.responseCode = protocol::messages::Response::RES_OK;

			this->requestLogger->logRequest(url, sStream.str(), queryResult);
		}
	}
	LOG(INFO, "test thread finished");
}

RequestLoggerTest::RequestLoggerTest(
	Config const & config,
	std::vector<std::string> & urls,
	uint32_t numIterations
):
	config(config),
	urls(urls),
	numIterations(numIterations),
	requestLogger()
{
	this->config.requestLoggerMaxQueueLength = this->getTotalRequestsNum();
	this->requestLogger.reset(new RequestLogger(this->config));
}

uint32_t RequestLoggerTest::getTotalRequestsNum() const
{
	return this->config.numWorkers.get() * this->urls.size() * this->numIterations;
}

void RequestLoggerTest::run()
{
	LOG(INFO, "starting test");

	boost::posix_time::ptime const startTime = boost::posix_time::microsec_clock::local_time();

	this->requestLogger->start();

	// Launch pool of worker threads
	boost::thread_group workerThreads;
	for (NumWorkers threadIdx = 0; threadIdx < this->config.numWorkers; threadIdx++)
	{
		workerThreads.create_thread(boost::bind(&RequestLoggerTest::workerFunction, this, threadIdx.get()));
	}
	// Wait for threads to finish
	workerThreads.join_all();
	boost::posix_time::time_duration const queuingDuration = boost::posix_time::microsec_clock::local_time() - startTime;

	// Stop request logger
	this->requestLogger->stop();
	boost::posix_time::time_duration const allDuration = boost::posix_time::microsec_clock::local_time() - startTime;

	RequestsStats const requestsStats = this->requestLogger->getRequestsStats();
	RequestLoggerThreadStats const requestLoggerThreadStats = this->requestLogger->getRequestLoggerThreadStats();

	ASSERT(requestsStats.numLogRequests == this->getTotalRequestsNum(), "num log requests inconsistency");
	ASSERT(requestLoggerThreadStats.numConnectionAttempts == 1, "errors occurred during logs db access",
		requestLoggerThreadStats.numConnectionAttempts);
	ASSERT(requestsStats.numDropped == 0, "expected no request dropping", requestsStats.numDropped);

	double const result = double(requestsStats.numLogRequests) / allDuration.total_milliseconds() * 1000;
	double const bandwidth = double(requestsStats.numRequestsBytes) / allDuration.total_milliseconds() * 1000;

	std::cout << "queuing duration: " << queuingDuration << "\n"
		"all duration: " << allDuration << "\n"
		"num logged requests: " << requestsStats.numLogRequests << "\n"
		"result: " << result << " reqs/s\n"
		"num bytes sent to db: " << requestsStats.numRequestsBytes << "\n"
		"bandwidth: " << bandwidth << " bytes/s\n"
		"num logger thread needed to sleep: " << requestsStats.numWaitsForRequests << "/" << requestsStats.numMoveToGetRequestsBuffer << " (" <<
			double(requestsStats.numWaitsForRequests) / requestsStats.numMoveToGetRequestsBuffer * 100 << "%)\n"
		"max idle time waiting for db commit: " << requestLoggerThreadStats.maxWaitForCommitDuration << "\n";

	LOG(INFO, "test result: " << result << " reqs/s, bandwidth: " << bandwidth << " bytes/s",
		queuingDuration, allDuration, requestsStats, requestLoggerThreadStats);

	this->writeResult(result);
}

void RequestLoggerTest::writeResult(double result)
{
	char const * path = "performance.txt";
	std::ofstream file;
	file.open(path, std::ios::trunc);
	ASSERT(file, "cannot open " << path);
	file << result << "\n";
	ASSERT(file, "error writing to " << path);
}

} // namespace testing
} // namespace backend
} // namespace safekiddo

void readUrls(std::vector<std::string> & urls, std::string const & urlPath, uint32_t numUrls)
{
	std::ifstream file;
	file.open(urlPath.c_str());
	ASSERT(file, "cannot open " << urlPath);
	for (uint32_t i = 0; i < numUrls; ++i)
	{
		std::string url;
		std::getline(file, url);
		urls.push_back(url);
	}
	ASSERT(file, "error reading from " << urlPath);
}

int32_t main(int32_t const argc, char ** argv)
{
	std::ofstream logFile;
	logFile.exceptions(std::ios::failbit | std::ios::badbit);
	logFile.open("requestLoggerTest.log", std::ios::app);
	su::StdLoggingDevice loggingDevice(su::LogLevel::DEBUG1, logFile);
	su::ScopedLoggingDeviceSetter logSetter(&loggingDevice);

	po::options_description description("Command line options");

	sb::NumWorkers numWorkers;
	sb::NumWorkers maxWorkersInCloud;
	std::string urlPath;
	std::string dbHostLogs;
	std::string dbNameLogs;
	std::string dbUser;
	std::string dbPassword;
	uint16_t dbPortLogs = 0;
	uint32_t numUrls = 0;
	uint32_t numIterations = 0;

	description.add_options()
		("help,h", "Produce help message")
		("urlFile,f", po::value<std::string>(&urlPath),
				"path to the file with urls")
		("workers,w",
			po::value<sb::NumWorkers>(&numWorkers)->default_value(16),
			"Number of worker threads")
		("maxWorkersInCloud,m",
			po::value<sb::NumWorkers>(&maxWorkersInCloud)->default_value(14),
			"Max number of worker threads allowed to connect with Webroot cloud in same time")
		("dbHostLogs", po::value<std::string>(&dbHostLogs)->default_value(""),
				"logs db server hostname")
		("dbPortLogs", po::value<uint16_t>(&dbPortLogs)->default_value(5432),
				"logs db server port")
		("dbNameLogs", po::value<std::string>(&dbNameLogs)->default_value("safekiddo_logs"),
				"logs db name")
		("dbUser", po::value<std::string>(&dbUser)->default_value("safekiddo"),
				"db user name")
		("dbPassword", po::value<std::string>(&dbPassword)->default_value("magic"),
				"db password")
		("numUrls,n", po::value<uint32_t>(&numUrls)->default_value(10000),
				"number of urls per worker")
		("numIterations", po::value<uint32_t>(&numIterations)->default_value(1),
				"number of iterations of urls logging")
		;

	po::variables_map variables;
	po::store(po::parse_command_line(argc, argv, description), variables);
	po::notify(variables);

	if (variables.count("help"))
	{
		std::cerr << description << std::endl;
		return 1;
	}
	if (variables.count("urlFile") == 0)
	{
		std::cerr << "Option urlFile is required" << std::endl;
		return 1;
	}

	sb::Config const config(
		numWorkers,
		maxWorkersInCloud,
		0, // numIoThreads, unused
		"", // serverUrl, unused
		"", // dbHost, unused
		0, // dbPort, unused
		dbHostLogs,
		dbPortLogs,
		"", // dbName, unused
		dbNameLogs,
		dbUser,
		dbPassword,
		"", // webrootConfigFilePath, unused
		0, // requestLoggerMaxQueueLength, unused
		"disable", // sslMode; ssl disabled
		"fakeOem",
		"fakeDevice",
		"fakeUid",
		false, // useCloud, unused
		false // useLocalDb, unused
	);

	std::vector<std::string> urls;
	readUrls(urls, urlPath, numUrls);

	sb::testing::RequestLoggerTest tester(config, urls, numIterations);
	tester.run();

	return 0;
}

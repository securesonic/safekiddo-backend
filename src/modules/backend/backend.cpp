#include <boost/program_options.hpp>

#include <stdint.h>
#include <fstream>

#include "server.h"
#include "backendMgmt.h"

#include "backendLib/currentTime.h"
#include "backendLib/config.h"
#include "backendLib/webrootOverrides.h"

#include "stats/statistics.h"
#include "utils/loggingDevice.h"
#include "utils/boostTime.h"
#include "version/version.h"

namespace sb = safekiddo::backend;
namespace su = safekiddo::utils;
namespace stats = safekiddo::statistics;
namespace po = boost::program_options;

#ifdef NDEBUG
#define DEFAULT_LOG_LEVEL	"INFO"
#else
#define DEFAULT_LOG_LEVEL	"DEBUG1"
#endif

int32_t main(int32_t const argc, char ** argv)
{
	po::options_description description("Command line options");

	sb::NumWorkers numWorkers;
	sb::NumWorkers maxWorkersInCloud;
	sb::NumIoThreads numIoThreads;
	std::string serverUrl;
	std::string dbHost;
	std::string dbHostLogs;
	std::string dbName;
	std::string dbNameLogs;
	std::string dbUser;
	std::string dbPassword;
	std::string webrootConfigFilePath;
	std::string currentTime;
	std::string sslMode;
	std::string webrootOem;
	std::string webrootDevice;
	std::string webrootUid;
	std::string logFilePath;
	std::string logLevel;
	std::string statisticsFilePath;
	uint32_t statisticsPrintingInterval = 0;
	uint32_t overridesUpdateInterval = 0;
	std::string mgmtFifoPath;
	uint16_t dbPort = 0;
	uint16_t dbPortLogs = 0;
	uint32_t requestLoggerMaxQueueLength = 0;

	description.add_options()
		("version,v", "Display version information")
		("help,h", "Produce help message")
		("workers,w",
			po::value<sb::NumWorkers>(&numWorkers)->default_value(16),
			"Number of worker threads")
		("maxWorkersInCloud,m",
			po::value<sb::NumWorkers>(&maxWorkersInCloud)->default_value(14),
			"Max number of worker threads allowed to connect with Webroot cloud in same time")
		("ioThreads,i",
			po::value<sb::NumIoThreads>(&numIoThreads)->default_value(1),
			"Number of io threads")
		("address,s", po::value<std::string>(&serverUrl)->default_value("tcp://*:7777"),
				"Server URL")
		("dbHost", po::value<std::string>(&dbHost)->default_value(""),
				"db server hostname")
		("dbPort", po::value<uint16_t>(&dbPort)->default_value(5432),
				"db server port")
		("dbHostLogs", po::value<std::string>(&dbHostLogs),
				"logs db server hostname")
		("dbPortLogs", po::value<uint16_t>(&dbPortLogs),
				"logs db server port")
		("dbName", po::value<std::string>(&dbName)->default_value("safekiddo"),
				"db name")
		("dbNameLogs", po::value<std::string>(&dbNameLogs)->default_value("safekiddo_logs"),
				"logs db name")
		("dbUser", po::value<std::string>(&dbUser)->default_value("safekiddo"),
				"db user name")
		("dbPassword", po::value<std::string>(&dbPassword)->default_value("magic"),
				"db password")
		("sslMode", po::value<std::string>(&sslMode)->default_value("disable"),
				"ssl mode for connection with databases; ssl disabled by default")
		("webrootConfigFilePath", po::value<std::string>(&webrootConfigFilePath)->default_value("bcsdk.cfg"),
				"path to webroot config file")
		("setCurrentTime", po::value<std::string>(&currentTime),
				"set static current local time for testing (YYYY-MM-DD hh:mm:ss)")
		("requestLoggerMaxQueueLength", po::value<uint32_t>(&requestLoggerMaxQueueLength)->default_value(10000),
				"max queue length of request logger")
		("webrootOem", po::value<std::string>(&webrootOem)->default_value("BrightCloudSdk"),
				"webroot oem")
		("webrootDevice", po::value<std::string>(&webrootDevice)->default_value("DeviceId_ardura"),
				"webroot device")
		("webrootUid", po::value<std::string>(&webrootUid)->default_value("ardura_tgf777blw"),
				"webroot uid")
		("disableWebrootCloud", "disable all access to webroot cloud")
		("disableWebrootLocalDb", "disable searching in local webroot database")
		("logFilePath", po::value<std::string>(&logFilePath)->default_value("backend.log"),
				"log file path")
		("logLevel", po::value<std::string>(&logLevel)->default_value(DEFAULT_LOG_LEVEL),
				"log level")
		("statisticsFilePath", po::value<std::string>(&statisticsFilePath)->default_value("backend.stats"),
			"statistics file path")
		("statisticsPrintingInterval", po::value<uint32_t>(&statisticsPrintingInterval)->default_value(30),
			"statistics printing interval in seconds")
		("overridesUpdateInterval", po::value<uint32_t>(&overridesUpdateInterval)->default_value(300),
			"webroot overrides update interval in seconds")
		("mgmtFifoPath", po::value<std::string>(&mgmtFifoPath)->default_value("backendMgmt.fifo"),
			"backend management fifo file path")
		;

	po::variables_map variables;
	po::store(po::parse_command_line(argc, argv, description), variables);
	po::notify(variables);

	if (variables.count("version"))
	{
		std::cerr << safekiddo::version::getDescription() << std::endl;
		return 1;
	}

	if (variables.count("help"))
	{
		std::cerr << description << std::endl;
		return 1;
	}

	boost::optional<su::LogLevelEnum> logLevelEnum = su::LoggingDevice::parseLogLevel(logLevel);
	if (!logLevelEnum)
	{
		std::cerr << "Invalid logLevel argument: " << logLevel << "\n";
		return 1;
	}

	su::FileLoggingDevice loggingDevice(logFilePath, logLevelEnum.get());
	su::ScopedLoggingDeviceSetter logSetter(&loggingDevice);

	if (!currentTime.empty())
	{
		boost::posix_time::ptime const localTime = su::parseDateTime(currentTime);
		if (localTime.is_special())
		{
			std::cerr << "Invalid setCurrentTime argument: " << currentTime << "\n";
			return 1;
		}
		sb::setCurrentTime(localTime);
	}

	if (variables.count("dbHostLogs") == 0)
	{
		dbHostLogs = dbHost;
	}
	if (variables.count("dbPortLogs") == 0)
	{
		dbPortLogs = dbPort;
	}

	bool const useCloud = !variables.count("disableWebrootCloud");
	bool const useLocalDb = !variables.count("disableWebrootLocalDb");

	sb::Config const config(
		numWorkers,
		maxWorkersInCloud,
		numIoThreads,
		serverUrl,
		dbHost,
		dbPort,
		dbHostLogs,
		dbPortLogs,
		dbName,
		dbNameLogs,
		dbUser,
		dbPassword,
		webrootConfigFilePath,
		requestLoggerMaxQueueLength,
		sslMode,
		webrootOem,
		webrootDevice,
		webrootUid,
		useCloud,
		useLocalDb
	);

	stats::StatisticsGatherer statsGatherer(statisticsFilePath, statisticsPrintingInterval);
	statsGatherer.startPrinting();

	sb::WebrootOverridesProvider webrootOverridesProvider(config, overridesUpdateInterval);

	sb::Server server(
		config,
		statsGatherer,
		webrootOverridesProvider
	);
	sb::BackendMgmt mgmtInterface(mgmtFifoPath, server, statsGatherer);

	mgmtInterface.start();

	LOG(INFO, "starting backend; version:\n" << safekiddo::version::getDescription())
	server.run(); // this blocks
	return 0;
}

#include <boost/program_options.hpp>

#include <iostream>
#include <stdint.h>

#include "utils/loggingDevice.h"
#include "utils/containerPrinter.h"
#include "utils/httpSender.h"

#include "webrootCpp/api.h"
#include "webrootCpp/config.h"
#include "version/version.h"

namespace su = safekiddo::utils;
namespace po = boost::program_options;

namespace safekiddo
{
namespace webrootQuery
{

class QueryTool
{
public:
	QueryTool(
		std::string const & webrootConfigFilePath,
		std::string const & webrootOem,
		std::string const & webrootDevice,
		std::string const & webrootUid,
		bool useCloud,
		bool useBcapCache,
		bool useLocalDb,
		bool useSafekiddo,
		bool machineOutput,
		std::string const & safekiddoHost,
		std::string const & safekiddoChildId
	);

	void run();

private:
	void queryLoop(webroot::WebrootConfig & webrootConfig);
	void dumpQueryResult(
		std::string const & url,
		std::string const & categorizations,
		std::string const & classificationSource,
		std::string const & isMostSpecific,
		std::string const & matchedUrl
	);
	void dumpQueryResult(std::string const & url, webroot::UrlInfo const & urlInfo);
	std::string getValueFromHeader(std::string const & header, std::string const & beginningString);
	void sendRequest(std::string const & server, std::string const & url, int & groupId, std::string & groupName);

	std::string const webrootConfigFilePath;
	std::string const webrootOem;
	std::string const webrootDevice;
	std::string const webrootUid;

	bool const useCloud;
	bool const useBcapCache;
	bool const useLocalDb;
	bool const useSafekiddo;
	bool const machineOutput;

	std::string const safekiddoHost;
	std::string const safekiddoChildId;
};

QueryTool::QueryTool(
	std::string const & webrootConfigFilePath,
	std::string const & webrootOem,
	std::string const & webrootDevice,
	std::string const & webrootUid,
	bool useCloud,
	bool useBcapCache,
	bool useLocalDb,
	bool useSafekiddo,
	bool machineOutput,
	std::string const & safekiddoHost,
	std::string const & safekiddoChildId
):
	webrootConfigFilePath(webrootConfigFilePath),
	webrootOem(webrootOem),
	webrootDevice(webrootDevice),
	webrootUid(webrootUid),
	useCloud(useCloud),
	useBcapCache(useBcapCache),
	useLocalDb(useLocalDb),
	useSafekiddo(useSafekiddo),
	machineOutput(machineOutput),
	safekiddoHost(safekiddoHost),
	safekiddoChildId(safekiddoChildId)
{
}

void QueryTool::run()
{
	webroot::WebrootConfig webrootConfig = webroot::BcReadConfig(this->webrootConfigFilePath);
	webrootConfig.setOem(this->webrootOem);
	webrootConfig.setDevice(this->webrootDevice);
	webrootConfig.setUid(this->webrootUid);

	webrootConfig.disableDbDownload();
	webrootConfig.disableRtu();

	LOG(INFO, "initializing webroot with config:");
	webroot::BcLogConfig(webrootConfig);

	if (!this->machineOutput)
	{
		std::cout << "Initializing webroot...\n";
	}
	webroot::BcInitialize(webrootConfig);
	webroot::BcWaitForDbLoadComplete(webrootConfig);

	this->queryLoop(webrootConfig);

	webroot::BcShutdown();

	LOG(INFO, "query tool stopped");
}

void QueryTool::dumpQueryResult(
	std::string const & url,
	std::string const & categorizations,
	std::string const & classificationSource,
	std::string const & isMostSpecific,
	std::string const & matchedUrl
)
{
	if (this->machineOutput)
	{
		std::cout
			<< url << "|"
			<< categorizations << "|" // categorization
			<< classificationSource << "|" // source
			<< isMostSpecific << "|" // a1cat
			<< matchedUrl << std::endl; // matchedUrl
	}
	else
	{
		std::cout
			<< "Got categorization of "<< url
			<< ": "<< categorizations
			<< " from " << classificationSource
			<< "; matchedUrl=" << matchedUrl
			<< ", claimsMostSpecific=" << isMostSpecific << std::endl;
	}
}

void QueryTool::dumpQueryResult(std::string const & url, webroot::UrlInfo const & urlInfo)
{
	this->dumpQueryResult(
		url,
		su::getStringFromStreamable(su::makeContainerPrinter(urlInfo.getCategorizations())),
		su::getStringFromStreamable(urlInfo.getClassificationSource()),
		su::getStringFromStreamable(urlInfo.isMostSpecific()),
		su::getStringFromStreamable(urlInfo.getMatchedUrl())
	);
}

void QueryTool::sendRequest(std::string const & server, std::string const & url, int & groupId, std::string & groupName)
{
	su::HttpSender httpSender(server);

	std::vector<std::string> otherHeaders;
	otherHeaders.push_back("Request: " + url);
	otherHeaders.push_back("UserAction: 2");
	otherHeaders.push_back("UserId: " + this->safekiddoChildId);

	std::vector<std::string> responseHeaders;
	std::string responseBody;

	httpSender.sendRequest("/cc", "GET", otherHeaders, "", responseHeaders, responseBody);

	std::string const groupIdString = "Category-group-id: ";
	std::string const groupNameString = "Category-group-name: ";
	std::string const resultString = "Result: ";
	int const unknownUserErrorCode = 300;

	for(auto &header : responseHeaders)
	{
		std::size_t const idPositon = header.find(groupIdString);
		std::size_t const namePosition = header.find(groupNameString);
		std::size_t const resultPosition = header.find(resultString);

		if (idPositon != std::string::npos)
		{
			groupId = boost::lexical_cast<int>(this->getValueFromHeader(header, groupIdString));
		}
		else if (namePosition != std::string::npos)
		{
			groupName = this->getValueFromHeader(header, groupNameString);
		}
		else if (resultPosition != std::string::npos)
		{
			if(boost::lexical_cast<int>(this->getValueFromHeader(header, resultString)) == unknownUserErrorCode)
			{
				std::cout << "Unknown childId: " << this->safekiddoChildId << std::endl;
				return;
			}
		}

	}
}

std::string QueryTool::getValueFromHeader(std::string const & header, std::string const & beginningString)
{
	// removing first part of header
	return header.substr(beginningString.size());
}

void QueryTool::queryLoop(webroot::WebrootConfig & webrootConfig)
{
	int bcapSocketDescriptor = 0;

	while (true)
	{
		if (!this->machineOutput)
		{
			std::cout << "Type a url: ";
		}
		std::string url;
		std::getline(std::cin, url);
		if (!std::cin)
		{
			break;
		}

		if (this->useLocalDb)
		{
			webrootConfig.enableLocalDb();
			webrootConfig.disableBcapCache();
			webrootConfig.disableBcap();
			webroot::BcSetConfig(webrootConfig);
			webroot::UrlInfo const localDbUrlInfo = webroot::BcLookup(url, bcapSocketDescriptor);

			this->dumpQueryResult(url, localDbUrlInfo);
		}

		if (this->useBcapCache)
		{
			webrootConfig.disableLocalDb();
			webrootConfig.enableBcapCache();
			webrootConfig.disableBcap();
			webroot::BcSetConfig(webrootConfig);
			webroot::UrlInfo const bcapCacheUrlInfo = webroot::BcLookup(url, bcapSocketDescriptor);

			this->dumpQueryResult(url, bcapCacheUrlInfo);
		}

		if (this->useCloud)
		{
			webrootConfig.disableLocalDb();
			webrootConfig.disableBcapCache();
			webrootConfig.enableBcap();
			webroot::BcSetConfig(webrootConfig);
			webroot::UrlInfo const bcapUrlInfo = webroot::BcLookup(url, bcapSocketDescriptor);

			this->dumpQueryResult(url, bcapUrlInfo);
		}

		if (this->useSafekiddo)
		{
			int groupId;
			std::string groupName;
			this->sendRequest(this->safekiddoHost, url, groupId, groupName);
			this->dumpQueryResult(
				url,
				"("+groupName+", "+std::to_string(groupId) + ")",
				this->safekiddoHost,
				"",
				""
			);
		}
	}
}

} // namespace webrootQuery
} // namespace safekiddo

#ifdef NDEBUG
#define DEFAULT_LOG_LEVEL	"INFO"
#else
#define DEFAULT_LOG_LEVEL	"DEBUG1"
#endif

int32_t main(int32_t const argc, char ** argv)
{
	po::options_description description("Command line options");

	std::string webrootConfigFilePath;
	std::string webrootOem;
	std::string webrootDevice;
	std::string webrootUid;
	std::string logFilePath;
	std::string logLevel;
	std::string safekiddoHost;
	std::string safekiddoChildId;

	description.add_options()
		("version,v", "Display version information")
		("help,h", "Produce help message")
		("webrootConfigFilePath", po::value<std::string>(&webrootConfigFilePath)->default_value("bcsdk.cfg"),
				"path to webroot config file")
		("webrootOem", po::value<std::string>(&webrootOem)->default_value("BrightCloudSdk"),
				"webroot oem")
		("webrootDevice", po::value<std::string>(&webrootDevice)->default_value("DeviceId_ardura"),
				"webroot device")
		("webrootUid", po::value<std::string>(&webrootUid)->default_value("ardura_tgf777blw"),
				"webroot uid")
		("disableWebrootCloud", "disable all access to webroot cloud")
		("disableWebrootBcapCache", "disable access to BCAP cache")
		("disableWebrootLocalDb", "disable searching in local webroot database")
		("disableSafekiddoQuery", "disable asking real Safekiddo instance")
		("machineOutput", "machine parseable output")
		("logFilePath", po::value<std::string>(&logFilePath)->default_value("webrootQuery.log"),
				"log file path")
		("logLevel", po::value<std::string>(&logLevel)->default_value(DEFAULT_LOG_LEVEL),
				"log level")
		("safekiddoHost", po::value<std::string>(&safekiddoHost)->default_value("SK-WS-459781308.eu-west-1.elb.amazonaws.com"),
			"host address of safekiddo instance")
		("safekiddoChildId", po::value<std::string>(&safekiddoChildId)->default_value("70514"),
			"Safekiddo child id")
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

	bool const useCloud = !variables.count("disableWebrootCloud");
	bool const useBcapCache = !variables.count("disableWebrootBcapCache");
	bool const useLocalDb = !variables.count("disableWebrootLocalDb");
	bool const useSafekiddo = !variables.count("disableSafekiddoQuery");
	bool const machineOutput = variables.count("machineOutput");

	safekiddo::webrootQuery::QueryTool queryTool(
		webrootConfigFilePath,
		webrootOem,
		webrootDevice,
		webrootUid,
		useCloud,
		useBcapCache,
		useLocalDb,
		useSafekiddo,
		machineOutput,
		safekiddoHost,
		safekiddoChildId
	);

	queryTool.run();

	return 0;
}

#include "backendLib/config.h"

namespace safekiddo
{
namespace backend
{

Config::Config(
	NumWorkers const numWorkers,
	NumWorkers const maxWorkersInCloud,
	NumIoThreads const numIoThreads,
	std::string const serverUrl,
	std::string const dbHost,
	uint16_t const dbPort,
	std::string const dbHostLogs,
	uint16_t const dbPortLogs,
	std::string const dbName,
	std::string const dbNameLogs,
	std::string const dbUser,
	std::string const dbPassword,
	std::string const webrootConfigFilePath,
	uint32_t const requestLoggerMaxQueueLength,
	std::string const sslMode,
	std::string const webrootOem,
	std::string const webrootDevice,
	std::string const webrootUid,
	bool const useCloud,
	bool const useLocalDb
):
	numWorkers(numWorkers),
	maxWorkersInCloud(maxWorkersInCloud),
	numIoThreads(numIoThreads),
	serverUrl(serverUrl),
	dbHost(dbHost),
	dbPort(dbPort),
	dbHostLogs(dbHostLogs),
	dbPortLogs(dbPortLogs),
	dbName(dbName),
	dbNameLogs(dbNameLogs),
	dbUser(dbUser),
	dbPassword(dbPassword),
	webrootConfigFilePath(webrootConfigFilePath),
	requestLoggerMaxQueueLength(requestLoggerMaxQueueLength),
	sslMode(sslMode),
	webrootOem(webrootOem),
	webrootDevice(webrootDevice),
	webrootUid(webrootUid),
	useCloud(useCloud),
	useLocalDb(useLocalDb)
{
}

} // namespace backend
} // namespace safekiddo

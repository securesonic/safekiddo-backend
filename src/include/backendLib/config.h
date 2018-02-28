#ifndef _SAFEKIDDO_BACKEND_CONFIG_H
#define _SAFEKIDDO_BACKEND_CONFIG_H

#include <stdint.h>
#include "utils/typedInteger.h"

namespace safekiddo
{
namespace backend
{

TYPED_INTEGER(NumWorkers, uint32_t);
TYPED_INTEGER(NumIoThreads, uint32_t);

struct Config
{
	Config(
		NumWorkers numWorkers,
		NumWorkers maxWorkersInCloud,
		NumIoThreads numIoThreads,
		std::string serverUrl,
		std::string dbHost,
		uint16_t dbPort,
		std::string dbHostLogs,
		uint16_t dbPortLogs,
		std::string dbName,
		std::string dbNameLogs,
		std::string dbUser,
		std::string dbPassword,
		std::string webrootConfigFilePath,
		uint32_t requestLoggerMaxQueueLength,
		std::string sslMode,
		std::string webrootOem,
		std::string webrootDevice,
		std::string webrootUid,
		bool useCloud,
		bool useLocalDb
	);

	NumWorkers numWorkers;
	NumWorkers maxWorkersInCloud;
	NumIoThreads numIoThreads;
	std::string serverUrl;
	std::string dbHost;
	uint16_t dbPort;
	std::string dbHostLogs;
	uint16_t dbPortLogs;
	std::string dbName;
	std::string dbNameLogs;
	std::string dbUser;
	std::string dbPassword;
	std::string webrootConfigFilePath;
	uint32_t requestLoggerMaxQueueLength;
	std::string sslMode;
	std::string webrootOem;
	std::string webrootDevice;
	std::string webrootUid;
	bool useCloud;
	bool useLocalDb;
};

} // namespace backend
} // namespace safekiddo

#endif // _SAFEKIDDO_BACKEND_CONFIG_H

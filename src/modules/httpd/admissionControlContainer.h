#ifndef ADMISSIONCONTROLCONTAINER_H_
#define ADMISSIONCONTROLCONTAINER_H_

#include <mutex>
#include <string>
#include <utility>
#include <thread>
#include <condition_variable>
#include <fstream>

#include <map>
#include <boost/regex.hpp>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "utils/assert.h"
#include "utils/logMacro.h"
#include "utils/loggingDevice.h"

bool operator<(in6_addr const & l, in6_addr const & r);
bool operator==(in6_addr const & l, in6_addr const & r);

namespace safekiddo
{
namespace webservice
{

class AdmissionControlContainer
{
public:
	AdmissionControlContainer(
		uint32_t connectionLimitAbsulute,
		uint32_t connectionLimitSlowDown,
		uint32_t invalidCredentialsPenalty,
		uint32_t slowDownDuration,
		uint32_t admissionControlPeriod,
		std::string const & admissionControlCustomSettingsFile
	);

	~AdmissionControlContainer();

	// Return:
	// 0 if access is permitted
	// -1 if access is unpermitted
	// > 0 if access is permitted after waiting of X milliseconds
	int getAccess(in6_addr const & ip, uint32_t & attempts);

	void addInvalidCredentialsPenalty(in6_addr const & ip);
	void mapClearLoop();
	void reloadCustomSettingsFile();

	static in6_addr inAddrV4ToV6(in_addr const inAddr);
	static bool stringToIp(std::string const & addressString, in6_addr & inAddr);
	static std::string ipToString(in6_addr const & ip);

	//returns true if successfull
	bool loadCustomSettingsMap(std::string const & filepath);

private:
	struct CustomSettings
	{
		uint32_t connectionLimitAbsolute;
		uint32_t connectionLimitSlowDown;
	};

	typedef std::map<in6_addr, uint32_t> IpMap;
	typedef std::map<in6_addr, CustomSettings> CustomSettingsMap;

	IpMap::iterator findOrCreate(in6_addr const & ip);
	CustomSettings getSettingsForIp(in6_addr const & ip) const;

	IpMap ipMap;
	CustomSettingsMap customSettingsMap;

	std::mutex mutable ipMapMutex;
	std::mutex mutable customSettingsMapMutex;
	std::mutex mutable sleepMutex;
	std::condition_variable mutable stopCondition;
	std::thread loopThread;

	uint32_t const connectionLimitAbsolute;
	uint32_t const connectionLimitSlowDown;
	uint32_t const invalidCredentialsPenalty;
	uint32_t const slowDownDuration; //in milliseconds
	uint32_t const admissionControlPeriod; //in seconds
	std::string const admissionControlCustomSettingsFile;

	bool stopped;
};

} // namespace webservice
} // namespace safekiddo

#endif /* ADMISSIONCONTROLCONTAINER_H_ */

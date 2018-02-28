#include "admissionControlContainer.h"

bool operator<(in6_addr const & l, in6_addr const & r)
{
	for(int i = 0; i < 4; i++)
	{
		if(l.s6_addr32[i] < r.s6_addr32[i])
		{
			return true;
		}
		if(l.s6_addr32[i] > r.s6_addr32[i])
		{
			return false;
		}
	}
	return false;
}

bool operator==(in6_addr const & l, in6_addr const & r)
{
	for(int i = 0; i < 4; i++)
	{
		if(l.s6_addr32[i] != r.s6_addr32[i])
		{
			return false;
		}
	}
	return true;
}

namespace safekiddo
{
namespace webservice
{

AdmissionControlContainer::AdmissionControlContainer(
	uint32_t connectionLimitAbsolute,
	uint32_t connectionLimitSlowDown,
	uint32_t invalidCredentialsPenalty,
	uint32_t slowDownDuration,
	uint32_t admissionControlPeriod,
	std::string const & admissionControlCustomSettingsFile
):
	connectionLimitAbsolute(connectionLimitAbsolute),
	connectionLimitSlowDown(connectionLimitSlowDown),
	invalidCredentialsPenalty(invalidCredentialsPenalty),
	slowDownDuration(slowDownDuration),
	admissionControlPeriod(admissionControlPeriod),
	admissionControlCustomSettingsFile(admissionControlCustomSettingsFile),
	stopped(false)
{
	bool successfull = this->loadCustomSettingsMap(this->admissionControlCustomSettingsFile);
	ASSERT(successfull, "CANNOT LOAD ADMISSION CONTROL CUSTOM SETTINGS MAP", this->admissionControlCustomSettingsFile);

	this->loopThread = std::thread(&AdmissionControlContainer::mapClearLoop, this);
}

AdmissionControlContainer::~AdmissionControlContainer()
{
	{
		std::lock_guard<std::mutex> lock(this->sleepMutex);
		this->stopped = true;
		this->stopCondition.notify_one();
	}
	this->loopThread.join();
}

in6_addr AdmissionControlContainer::inAddrV4ToV6(in_addr const inAddr)
{
	in6_addr toReturn;
	for(int i = 0; i < 5; i++)
	{
		toReturn.s6_addr16[i] = 0;
	}
	toReturn.s6_addr16[5] = -1;
	std::memcpy(&toReturn.s6_addr16[6], &inAddr.s_addr, 4);

	DASSERT(IN6_IS_ADDR_V4MAPPED(&toReturn), "IPv6 address converted from IPv4 address in not mapped correctly");
	return toReturn;
}

bool AdmissionControlContainer::stringToIp(std::string const & addressString, in6_addr & inAddr)
{
	sockaddr_in sockaddrV4;
	sockaddr_in6 sockaddrV6;

	int success = inet_pton(AF_INET, addressString.c_str(), &sockaddrV4.sin_addr);
	if(success == 1)
	{
		inAddr = AdmissionControlContainer::inAddrV4ToV6(sockaddrV4.sin_addr);
	}
	else
	{
		success = inet_pton(AF_INET6, addressString.c_str(), &sockaddrV6.sin6_addr);
		if(success == 1)
		{
			inAddr = sockaddrV6.sin6_addr;
		}
	}
	return success == 1;
}

std::string AdmissionControlContainer::ipToString(in6_addr const & ip)
{
	char buffer[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, &ip, buffer, sizeof(buffer));
	return buffer;
}

AdmissionControlContainer::CustomSettings AdmissionControlContainer::getSettingsForIp(in6_addr const & ip) const
{
	std::lock_guard<std::mutex> lock(this->customSettingsMapMutex);
	CustomSettingsMap::const_iterator customSettingsIterator = this->customSettingsMap.find(ip);

	if(customSettingsIterator == this->customSettingsMap.end())
	{
		return CustomSettings{this->connectionLimitAbsolute, this->connectionLimitSlowDown};
	}
	else
	{
		return customSettingsIterator->second;
	}
}

int AdmissionControlContainer::getAccess(in6_addr const & ip, uint32_t & attempts)
{
	CustomSettings const customSettings = this->getSettingsForIp(ip);

	std::lock_guard<std::mutex> lock(this->ipMapMutex);
	IpMap::iterator addressDataIterator = this->findOrCreate(ip);

	addressDataIterator->second++;
	attempts = addressDataIterator->second;
	if(addressDataIterator->second > customSettings.connectionLimitAbsolute)
	{
		return -1;
	}
	else if(addressDataIterator->second > customSettings.connectionLimitSlowDown)
	{
		return this->slowDownDuration;
	}

	return 0;
}

void AdmissionControlContainer::addInvalidCredentialsPenalty(in6_addr const & ip)
{
	std::lock_guard<std::mutex> lock(this->ipMapMutex);
	IpMap::iterator addressDataIterator = this->findOrCreate(ip);
	addressDataIterator->second += this->invalidCredentialsPenalty;
}

void AdmissionControlContainer::mapClearLoop()
{
	while (true)
	{
		{
			std::lock_guard<std::mutex> lock(this->ipMapMutex);
			this->ipMap.clear();
		}
		std::unique_lock<std::mutex> uniqueSleepLock(this->sleepMutex);
		if (this->stopCondition.wait_for(uniqueSleepLock, std::chrono::seconds(this->admissionControlPeriod), [this](){return this->stopped;}))
		{
			break;
		}
	}
}

void AdmissionControlContainer::reloadCustomSettingsFile()
{
	bool successfull = this->loadCustomSettingsMap(this->admissionControlCustomSettingsFile);
	if(successfull)
	{
		// there are new limits so clear existing statistics
		{
			std::lock_guard<std::mutex> lock(this->ipMapMutex);
			this->ipMap.clear();
		}
		LOG(INFO, "admission control custom settings file reload complete", this->admissionControlCustomSettingsFile);
	}
	else
	{
		LOG(ERROR, "CANNOT LOAD ADMISSION CONTROL CUSTOM SETTINGS MAP", this->admissionControlCustomSettingsFile);
	}
}

bool AdmissionControlContainer::loadCustomSettingsMap(std::string const & filepath)
{
	CustomSettingsMap newCustomSettingsMap;
	std::ifstream myFile;
	myFile.open(filepath);
	bool successfull = true;

	boost::regex lineRegex("[0-9.:]+\\s[0-9]+\\s[0-9]+");
	if(myFile.is_open())
	{
		std::string line = "";
		uint32_t lineNum = 0;
		while(std::getline(myFile, line))
		{
			lineNum++;

			if(!boost::regex_match(line, lineRegex))
			{
				LOG(ERROR, "Admission control custom settings file parsing failed", lineNum, line);
				successfull = false;
				break;
			}

			std::istringstream stream(line);
			std::string ipString;
			stream >> ipString;

			in6_addr ip;
			if(!AdmissionControlContainer::stringToIp(ipString, ip))
			{
				LOG(ERROR, "Cannot parse ip address", lineNum, ipString);
				successfull = false;
				break;
			}
			uint32_t connectionLimitAbsolute;
			uint32_t connectionLimitSlowDown;
			stream >> connectionLimitAbsolute >> connectionLimitSlowDown;

			auto emplaced = newCustomSettingsMap.emplace(ip, CustomSettings{connectionLimitAbsolute, connectionLimitSlowDown});

			if(!emplaced.second)
			{
				LOG(ERROR, "Address IP exists in file two (or more) times", lineNum, ipString);
				successfull = false;
				break;
			}
		}
	}
	else
	{
		LOG(ERROR, "Cannot open admission control custom settings file", filepath);
		successfull = false;
	}
	myFile.close();

	if(successfull)
	{
		std::lock_guard<std::mutex> lock(this->customSettingsMapMutex);
		this->customSettingsMap.swap(newCustomSettingsMap);
	}
	return successfull;
}

AdmissionControlContainer::IpMap::iterator AdmissionControlContainer::findOrCreate(in6_addr const & ip)
{
	return this->ipMap.emplace(ip, 0).first;
}

} // namespace webservice
} // namespace safekiddo

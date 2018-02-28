/*
 * config.cpp
 *
 *  Created on: 3 Apr 2014
 *      Author: zytka
 */

#include "webrootCpp/config.h"
#include "sdkcfg.h"
#include <string.h>

namespace safekiddo
{
namespace webroot
{

WebrootConfig::WebrootConfig(sConfigValsStruct const & rawConfig):
	rawConfig(rawConfig)
{
}

WebrootConfig & WebrootConfig::enableRtu()
{
	this->rawConfig.bDoRtu = true;
	return *this;
}

WebrootConfig & WebrootConfig::disableRtu()
{
	this->rawConfig.bDoRtu = false;
	return *this;
}

WebrootConfig & WebrootConfig::setRtuIntervalMin(uint32_t const interval)
{
	this->rawConfig.nUpdateIntervalMins = interval;
	return *this;
}

WebrootConfig & WebrootConfig::setBcapCacheTtlSec(uint32_t const ttl)
{
	this->rawConfig.nServiceCacheTtl = ttl;
	return *this;
}

WebrootConfig & WebrootConfig::enableBcap()
{
	this->rawConfig.bDoBcap = true;
	return *this;
}

WebrootConfig & WebrootConfig::disableBcap()
{
	this->rawConfig.bDoBcap = false;
	return *this;
}

WebrootConfig & WebrootConfig::enableBcapCache()
{
	this->rawConfig.checkBcapCache = true;
	return *this;
}

WebrootConfig & WebrootConfig::disableBcapCache()
{
	this->rawConfig.checkBcapCache = false;
	return *this;
}

WebrootConfig & WebrootConfig::enableDbDownload()
{
	this->rawConfig.bDownloadDatabase = true;
	return *this;
}

WebrootConfig & WebrootConfig::disableDbDownload()
{
	this->rawConfig.bDownloadDatabase = false;
	return *this;
}

WebrootConfig & WebrootConfig::enableLocalDb()
{
	this->rawConfig.checkLocalDb = true;
	return *this;
}

WebrootConfig & WebrootConfig::disableLocalDb()
{
	this->rawConfig.checkLocalDb = false;
	return *this;
}

WebrootConfig & WebrootConfig::setBcapServer(std::string const bcapServer)
{
	::strncpy(this->rawConfig.szBcapServer, bcapServer.c_str(), 255);
	return *this;
}

WebrootConfig & WebrootConfig::setOem(std::string const oem)
{
	::strncpy(this->rawConfig.szOem, oem.c_str(), 255);
	return *this;
}

WebrootConfig & WebrootConfig::setDevice(std::string const device)
{
	::strncpy(this->rawConfig.szDevice, device.c_str(), 255);
	return *this;
}

WebrootConfig & WebrootConfig::setUid(std::string const uid)
{
	::strncpy(this->rawConfig.szUid, uid.c_str(), 255);
	return *this;
}

WebrootConfig & WebrootConfig::setDbDir(std::string const dbDir)
{
	::strncpy(this->rawConfig.szLocalDatabaseDir, dbDir.c_str(), 255);
	return *this;
}

sConfigValsStruct const & WebrootConfig::getRawConfig() const
{
	return this->rawConfig;
}

} // namespace webroot
} // namespace safekiddo

/*
 * config.h
 *
 *  Created on: 3 Apr 2014
 *      Author: zytka
 */

#ifndef _SAFEKIDDO_CONFIG_H_
#define _SAFEKIDDO_CONFIG_H_

#include <boost/noncopyable.hpp>
#include <stdint.h>
#include <string>
#include "sdkcfg.h"

namespace safekiddo
{
namespace webroot
{

class WebrootConfig
{
public:
	explicit WebrootConfig(sConfigValsStruct const & rawConfig);

	WebrootConfig & enableRtu();
	WebrootConfig & disableRtu();
	WebrootConfig & setRtuIntervalMin(uint32_t const interval);
	WebrootConfig & setBcapCacheTtlSec(uint32_t const ttl);
	WebrootConfig & enableBcap();
	WebrootConfig & disableBcap();
	WebrootConfig & enableBcapCache();
	WebrootConfig & disableBcapCache();
	WebrootConfig & enableDbDownload();
	WebrootConfig & disableDbDownload();
	WebrootConfig & enableLocalDb();
	WebrootConfig & disableLocalDb();
	WebrootConfig & setBcapServer(std::string const dbServer);
	WebrootConfig & setOem(std::string const oem);
	WebrootConfig & setDevice(std::string const device);
	WebrootConfig & setUid(std::string const uid);
	WebrootConfig & setDbDir(std::string const uid);

	sConfigValsStruct const & getRawConfig() const;

private:
	sConfigValsStruct rawConfig;
};

} // namespace webroot
} // namespace safekiddo



#endif /* _SAFEKIDDO_CONFIG_H_ */

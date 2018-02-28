/*
 * api.h
 *
 *  Created on: 12 Feb 2014
 *      Author: zytka
 */

#ifndef _SAFEKIDDO_API_H_
#define _SAFEKIDDO_API_H_

#include <limits>
#include "urlInfo.h"
#include "utils/typedInteger.h"

namespace safekiddo
{

namespace statistics
{
class StatisticsGatherer;
}

namespace webroot
{

class WebrootConfig;

// does not wait for db load to complete
void BcInitialize(WebrootConfig const & config);
void BcWaitForDbLoadComplete(WebrootConfig const & config);
void BcShutdown();

void BcLogConfig(WebrootConfig const & config);
void BcSetConfig(WebrootConfig const & config);

void BcSdkInit();
void BcStartRtu();
void BcStopRtu();

void BcStartStatisticsSending(statistics::StatisticsGatherer & statsGatherer);
void BcStopStatisticsSending();

// FIXME: zytka, 2014-04-28, we should hide int & under some object so that details are hidden in one place
// like: webroot expects socket == 0 for uninitialized socket
UrlInfo BcLookup(std::string const & url, int & socketDescriptor, uint32_t maxCloudThreads = std::numeric_limits<uint32_t>::max());

WebrootConfig BcReadConfig(std::string const webrootConfigFilePath);

} // namespace webroot
} // namespace safekiddo

#endif /* _SAFEKIDDO_API_H_ */

#ifndef _SAFEKIDDO_BACKEND_HIT_BY_SOURCE_STATISTICS_SENDER_H
#define _SAFEKIDDO_BACKEND_HIT_BY_SOURCE_STATISTICS_SENDER_H

#include <mutex>
#include "stats/statistics.h"
#include "webrootCpp/urlInfo.h"

namespace stats = safekiddo::statistics;

namespace safekiddo
{
namespace backend
{

using webroot::UrlInfo;

class HitBySourceStatisticsSender: public stats::StatisticsSender
{
public:
	explicit HitBySourceStatisticsSender(stats::StatisticsGatherer & statsGatherer);
	void addFromClassificationSource(UrlInfo::ClassificationSource classificationSource);
	void addFromWebrootOverride();

protected:
	virtual std::vector<stats::StatisticsMessage> getMessages() override;

private:
	std::mutex allPrivateVariableMutex;
	uint32_t unset;
	uint32_t localIp;
	uint32_t rtuCache;
	uint32_t localDb;
	uint32_t bcapCache;
	uint32_t bcap;
	uint32_t webrootOverride;
};

} // namespace backend
} // namespace safekiddo

#endif // _SAFEKIDDO_BACKEND_HIT_BY_SOURCE_STATISTICS_SENDER_H

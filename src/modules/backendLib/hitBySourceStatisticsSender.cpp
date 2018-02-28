#include "backendLib/hitBySourceStatisticsSender.h"

namespace safekiddo
{
namespace backend
{

HitBySourceStatisticsSender::HitBySourceStatisticsSender(
	stats::StatisticsGatherer & statsGatherer
):
	stats::StatisticsSender(statsGatherer),
	unset(0),
	localIp(0),
	rtuCache(0),
	localDb(0),
	bcapCache(0),
	bcap(0),
	webrootOverride(0)
{
}

void HitBySourceStatisticsSender::addFromClassificationSource(UrlInfo::ClassificationSource classificationSource)
{
	std::lock_guard<std::mutex> lock(this->allPrivateVariableMutex);
	switch(classificationSource)
	{
	case UNSET:
		this->unset++;
		return;
	case LOCAL_IP:
		this->localIp++;
		return;
	case RTU_CACHE:
		this->rtuCache++;
		return;
	case LOCAL_DB:
		this->localDb++;
		return;
	case BCAP_CACHE:
		this->bcapCache++;
		return;
	case BCAP:
		this->bcap++;
		return;
	}
	FAILURE("got unexpected decision value: " << classificationSource);
}

void HitBySourceStatisticsSender::addFromWebrootOverride()
{
	std::lock_guard<std::mutex> lock(this->allPrivateVariableMutex);
	this->webrootOverride++;
}

std::vector<stats::StatisticsMessage> HitBySourceStatisticsSender::getMessages()
{
	std::lock_guard<std::mutex> lock(this->allPrivateVariableMutex);
	std::vector<stats::StatisticsMessage> statsMessages;
	statsMessages.push_back(stats::StatisticsMessage(stats::Unset, this->unset));
	statsMessages.push_back(stats::StatisticsMessage(stats::LocalIp, this->localIp));
	statsMessages.push_back(stats::StatisticsMessage(stats::RtuCache, this->rtuCache));
	statsMessages.push_back(stats::StatisticsMessage(stats::LocalDb, this->localDb));
	statsMessages.push_back(stats::StatisticsMessage(stats::BcapCache, this->bcapCache));
	statsMessages.push_back(stats::StatisticsMessage(stats::Bcap, this->bcap));
	statsMessages.push_back(stats::StatisticsMessage(stats::WebrootOverride, this->webrootOverride));
	return statsMessages;
}

} // namespace backend
} // namespace safekiddo

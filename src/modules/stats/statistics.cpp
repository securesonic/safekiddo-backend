#include <stats/statistics.h>
#include <sstream>

namespace safekiddo
{
namespace statistics
{

std::ostream & operator<<(std::ostream & os, StatisticsTypeEnum type)
{
	switch (type)
	{
	case BcBcapMap:
		return os << "bcBcapMap";
	case BcRtuMap:
		return os << "bcRtuMap";
	case Unset:
		return os << "unset";
	case LocalIp:
		return os << "localIp";
	case RtuCache:
		return os << "rtuCache";
	case LocalDb:
		return os << "localDb";
	case BcapCache:
		return os << "bcapCache";
	case Bcap:
		return os << "bcap";
	case WebrootOverride:
		return os << "webrootOverride";
	}
	FAILURE("invalid statistics type: " << static_cast<int>(type));
}

////////////////// StatisticsMessage ////////////////////////////
StatisticsMessage::StatisticsMessage(StatisticsTypeEnum type, value_t value):
	time(system_clock::now()),
	type(type),
	value(value)
{
}

StatisticsTypeEnum StatisticsMessage::getType() const
{
	return this->type;
}

value_t StatisticsMessage::getValue() const
{
	return this->value;
}

system_clock::time_point StatisticsMessage::getTime() const
{
	return this->time;
}

////////////////// StatisticsContainer ////////////////////////////
StatisticsContainer::StatisticsContainer():
	maxPair(system_clock::now(), 0),
	lastPair(system_clock::now(), 0),
	recentlyPrintedLastValue(0)
{
}

std::pair<system_clock::time_point, value_t> StatisticsContainer::last() const
{
	return this->lastPair;
}

std::pair<system_clock::time_point, value_t> StatisticsContainer::max() const
{
	return this->maxPair;
}

value_t StatisticsContainer::differenceBetweenPrintings() const
{

	return this->last().second - this->recentlyPrintedLastValue;
}

void StatisticsContainer::resetRecent()
{
	this->recentlyPrintedLastValue = this->last().second;
}

void StatisticsContainer::add(system_clock::time_point time, value_t value)
{
	this->lastPair = std::make_pair(time, value);
	if(value > this->maxPair.second)
	{
		this->maxPair = this->lastPair;
	}
}

////////////////// StatisticsGatherer ////////////////////////////
StatisticsGatherer::StatisticsGatherer(std::string const & filePath, unsigned int printInterval):
	statsFilePath(filePath),
	printInterval(printInterval),
	stopped(true),
	forceReopenRequired(false)
{
	this->openStatsFile();
}

StatisticsGatherer::~StatisticsGatherer()
{
	{
		std::lock_guard<std::mutex> lock(this->sleepMutex);
		this->stopped = true;
		this->stopCondition.notify_one();
	}
	this->printStatisticsLoopThread.join();
}

void StatisticsGatherer::reopen()
{
	std::lock_guard<std::mutex> lock(this->sleepMutex);
	this->forceReopenRequired = true;
	this->stopCondition.notify_one();
}

void StatisticsGatherer::openStatsFile()
{
	this->outfile.clear();
	this->outfile.open(this->statsFilePath.c_str(), std::ios::app);
	// FIXME: SAF-294: backend could work without stats
	ASSERT(this->outfile.is_open(), "cannot open statistics file " << this->statsFilePath);
	this->outfile << "--- STATS START ---\n";
}

void StatisticsGatherer::closeStatsFile()
{
	this->outfile << "--- STATS END ---\n";
	this->outfile.close();
}

const std::string StatisticsGatherer::createLogLine(
	StatisticsTypeEnum type,
	system_clock::time_point time,
	std::string const & valueName,
	value_t value
)
{
	time_t timeT = system_clock::to_time_t(time);
	struct tm tm;
	::localtime_r(&timeT, &tm);

	std::string timeStr;
	timeStr.resize(80);
	size_t len = std::strftime(&timeStr[0], timeStr.size(), "%Y-%m-%d %H:%M:%S", &tm);
	DASSERT(len > 0, "strftime failed");
	timeStr.resize(len);

	std::ostringstream ss;
	ss << "[" << type << "] " << valueName << ": "<< value << " at " << timeStr << "\n";
	return ss.str();
}

void StatisticsGatherer::printStatisticsLoop()
{
	LOG(INFO, "statistics printing thread started");
	this->outfile << "Statistics printing started" << std::endl;

	while (true)
	{
		this->printAllStats();
		this->outfile.flush();

		std::unique_lock<std::mutex> uniqueSleepLock(this->sleepMutex);
		if (this->stopCondition.wait_for(
			uniqueSleepLock,
			seconds(this->printInterval),
			[this](){return this->forceReopenRequired || this->stopped;}
		))
		{
			if (this->stopped)
			{
				break;
			}
			if (this->forceReopenRequired)
			{
				this->forceReopenRequired = false;
				this->closeStatsFile();
				this->openStatsFile();
			}
		}
	}

	this->printAllStats();
	this->outfile << "Statistics printing stopped" << std::endl;
	LOG(INFO, "statistics printing thread finished");
}

void StatisticsGatherer::printAllStats()
{
	std::lock_guard<std::mutex> lock(this->statisticsMapMutex);
	for(auto it = this->statisticsMap.begin(); it != this->statisticsMap.end(); it++)
	{
		StatisticsTypeEnum type = it->first;
		StatisticsContainer & statisticsContainer = it->second;

		auto lastAddedToContainer = statisticsContainer.last();
		this->outfile << StatisticsGatherer::createLogLine(type, lastAddedToContainer.first, "last value", lastAddedToContainer.second);

		auto maximumValue = statisticsContainer.max();
		this->outfile << StatisticsGatherer::createLogLine(type, maximumValue.first, "maximum value", maximumValue.second);

		value_t differenceBetweenPrintingsValue = statisticsContainer.differenceBetweenPrintings();
		this->outfile << StatisticsGatherer::createLogLine(type, system_clock::now(), "difference since last printing", differenceBetweenPrintingsValue);

		statisticsContainer.resetRecent();
	}
}

void StatisticsGatherer::receiveStatistics(std::vector<StatisticsMessage> const & values)
{
	DASSERT(!this->stopped, "already stopped - cannot receive statistics");

	std::lock_guard<std::mutex> lock(this->statisticsMapMutex);
	for(auto it = values.begin(); it != values.end(); it++)
	{
		this->statisticsMap[it->getType()].add(it->getTime(), it->getValue());
	}
}

void StatisticsGatherer::startPrinting()
{
	DASSERT(this->stopped, "already started");
	this->stopped = false;
	this->printStatisticsLoopThread = std::thread(&StatisticsGatherer::printStatisticsLoop, this);
}

////////////////// StatisticsSender ////////////////////////////
StatisticsSender::StatisticsSender(
	StatisticsGatherer & statsGatherer,
	unsigned int interval
):
	statsGatherer(statsGatherer),
	interval(interval),
	stopped(false)
{
}

void StatisticsSender::sendStatisticsLoop()
{
	while (true)
	{
		this->statsGatherer.receiveStatistics(this->getMessages());

		std::unique_lock<std::mutex> uniqueSleepLock(this->sleepMutex);
		if (this->stopCondition.wait_for(uniqueSleepLock, seconds(this->interval), [this](){return this->stopped;}))
		{
			break;
		}
	}
}

void StatisticsSender::startStatisticsSending()
{
	this->sendStatisticsLoopThread = std::thread(&StatisticsSender::sendStatisticsLoop, this);
}

StatisticsSender::~StatisticsSender()
{
	{
		std::lock_guard<std::mutex> lock(this->sleepMutex);
		this->stopped = true;
		this->stopCondition.notify_one();
	}
	this->sendStatisticsLoopThread.join();
}

} // namespace statistics
} // namespace safekiddo

#ifndef STATISTIC_H_
#define STATISTIC_H_

#ifdef NDEBUG
#define STATS_SEND_INTERVAL	5
#else
#define STATS_SEND_INTERVAL	1
#endif

#include <thread>
#include <map>
#include <string>
#include <chrono>
#include <vector>
#include <utility>
#include <mutex>
#include <condition_variable>
#include <ctime>
#include <fstream>

#include "utils/loggingDevice.h"
#include "utils/assert.h"

namespace safekiddo
{
namespace statistics
{

using namespace std::chrono;
using value_t = int32_t;

enum StatisticsTypeEnum
{
	BcBcapMap,
	BcRtuMap,
	Unset,
	LocalIp,
	RtuCache,
	LocalDb,
	BcapCache,
	Bcap,
	WebrootOverride
};

std::ostream & operator<<(std::ostream & os, StatisticsTypeEnum type);

class StatisticsMessage
{
public:
	StatisticsMessage(StatisticsTypeEnum type, value_t value);

	StatisticsTypeEnum getType() const;
	value_t getValue() const;
	system_clock::time_point getTime() const;

private:
	system_clock::time_point time;
	StatisticsTypeEnum type;
	value_t value;
};

// StatisticsContainer is container for
// statistics data of one type
// It's stored and regularly filled in StatisticsGatherer
class StatisticsContainer
{
public:
	typedef std::pair<system_clock::time_point, value_t> StatisticsValuePair;
	StatisticsContainer();

	StatisticsValuePair last() const;
	StatisticsValuePair max() const;
	value_t differenceBetweenPrintings() const;
	void resetRecent();

	void add(system_clock::time_point time, value_t value);

private:
	StatisticsValuePair maxPair;
	StatisticsValuePair lastPair;
	value_t recentlyPrintedLastValue;
};

// Main statistics class
// This is singleton which store
// and collect all statistics data
class StatisticsGatherer
{
public:
	StatisticsGatherer(std::string const & filePath, unsigned int printInterval);
	~StatisticsGatherer();

	void startPrinting();
	void receiveStatistics(std::vector<StatisticsMessage> const & values);

	void reopen();

private:
	static const std::string createLogLine(
		StatisticsTypeEnum type,
		system_clock::time_point time,
		std::string const & valueName,
		value_t value
	);
	void printStatisticsLoop();
	void printAllStats();

	void openStatsFile();
	void closeStatsFile();

	std::map<StatisticsTypeEnum, StatisticsContainer> statisticsMap;
	std::thread printStatisticsLoopThread;
	std::mutex statisticsMapMutex;
	std::mutex sleepMutex;
	std::condition_variable stopCondition;
	std::ofstream outfile;
	std::string const statsFilePath;
	unsigned int printInterval;
	bool stopped;
	bool forceReopenRequired;
};

// StatisticsSender is parent class
// of all objects which send data
// to the StatisticsGatherer
class StatisticsSender
{
public:
	StatisticsSender(
		StatisticsGatherer & statsGatherer,
		unsigned int interval = STATS_SEND_INTERVAL
	);
	virtual ~StatisticsSender();

	void startStatisticsSending();

protected:
	// This method must be overridden with function which
	// generates vector with statistics messages of
	// the object.
	virtual std::vector<StatisticsMessage> getMessages() = 0;

private:
	void sendStatisticsLoop();

	StatisticsGatherer & statsGatherer;
	unsigned int interval;

	std::thread sendStatisticsLoopThread;
	std::mutex sleepMutex;
	std::condition_variable stopCondition;
	bool stopped;
};


} // namespace statistics
} // namespace safekiddo

#endif /* STATISTIC_H_ */

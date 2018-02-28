#ifndef _SAFEKIDDO_BACKEND_REQUEST_LOGGER_H
#define _SAFEKIDDO_BACKEND_REQUEST_LOGGER_H

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <list>
#include <string>

#include "dbClient.h"
#include "config.h"
#include "backendQueryResult.h"

#include "utils/queue.h"

namespace safekiddo
{
namespace backend
{

struct RequestsStats
{
	RequestsStats();

	uint64_t numLogRequests;
	uint64_t numDropped;
	uint64_t numWaitsForRequests;
	uint64_t numMoveToGetRequestsBuffer;
	uint64_t numRequestsBytes;
};

std::ostream & operator<<(std::ostream & out, RequestsStats const & self);

class RequestsContainer
{
public:
	RequestsContainer(Config const & config);

	~RequestsContainer();

	// Appends at most maxRequests requests to copyData. May block if mayBlock is true until there
	// is some request, otherwise is not-blocking. Returns the number of requests appended to
	// copyData.
	uint32_t getRequests(bool mayBlock, std::string & copyData, uint32_t maxRequests);

	void putRequest(std::string const & request);

	void dropAllRequests();

	// Makes getRequests() return if it would block due to lack of requests. If it is already
	// blocking another thread then the method wakes it up.
	void requestStop();
	bool isStopRequested() const;

	RequestsStats const & getStats() const;

private:
	boost::mutex mutable requestsMutex;
	boost::condition_variable mutable requestAvailable;
	utils::Queue<std::string> requests; // requests to be logged queue
	bool stopRequested;

	// protected by requestsMutex
	RequestsStats stats;

	Config const & config; // kept by reference

	// Below is not protected by mutex.
	// A list for storing requests taken out of this->requests. Motivation is to avoid blocking
	// background thread on the mutex, which is taken by backend worker threads on each
	// logRequest().
	std::list<std::string> getRequestsBuffer;
};

struct RequestLoggerThreadStats
{
	RequestLoggerThreadStats();

	uint32_t numConnectionAttempts;
	boost::posix_time::time_duration maxWaitForCommitDuration;
};

std::ostream & operator<<(std::ostream & out, RequestLoggerThreadStats const & self);

// thread-safe
class RequestLogger
{
public:
	explicit RequestLogger(Config const & config);

	void start();

	// waits until all requests are committed
	void stop();

	void logRequest(
		std::string const & requestUrl,
		db::Uuid const & childUuid,
		BackendQueryResult const & result
	);

	RequestsStats const & getRequestsStats() const;
	RequestLoggerThreadStats const & getRequestLoggerThreadStats() const;

private:
	class ThreadStatsHolder
	{
	public:
		void incConnectionAttempts();
		void addWaitForCommitDuration(boost::posix_time::time_duration const & duration);

		RequestLoggerThreadStats const & getStats() const;

	private:
		boost::mutex mutable threadStatsMutex;
		RequestLoggerThreadStats stats;
	};

	enum LoggingRetcode
	{
		STOP_REQUESTED,
		DB_FAILURE
	};

	bool createConnectionPool(std::vector<boost::shared_ptr<db::DBConnection> > & connectionPool) const;

	void loggerThreadFunction();

	bool waitForCommit(db::DBConnection const & dbConnection);

	LoggingRetcode loggingLoop(std::vector<boost::shared_ptr<db::DBConnection> > const & connectionPool);

	Config const config;

	boost::scoped_ptr<boost::thread> loggerThread;

	RequestsContainer requestsContainer;

	ThreadStatsHolder statsHolder;
};

} // namespace backend
} // namespace safekiddo

#endif // _SAFEKIDDO_BACKEND_REQUEST_LOGGER_H

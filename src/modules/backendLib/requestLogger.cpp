#include "backendLib/requestLogger.h"
#include "backendLib/currentTime.h"

#include "utils/assert.h"
#include "utils/alerts.h"

#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <sstream>

namespace su = safekiddo::utils;

namespace safekiddo
{
namespace backend
{

/****************************** RequestsStats ********************************/

RequestsStats::RequestsStats():
	numLogRequests(0),
	numDropped(0),
	numWaitsForRequests(0),
	numMoveToGetRequestsBuffer(0),
	numRequestsBytes(0)
{
}

std::ostream & operator<<(std::ostream & out, RequestsStats const & self)
{
	return out << "RequestsStats["
		"numLogRequests: " << self.numLogRequests << ", "
		"numDropped: " << self.numDropped << ", "
		"numWaitsForRequests: " << self.numWaitsForRequests << ", "
		"numMoveToGetRequestsBuffer: " << self.numMoveToGetRequestsBuffer << ", "
		"numRequestsBytes: " << self.numRequestsBytes << "]";
}

/**************************** RequestsContainer ******************************/

RequestsContainer::RequestsContainer(Config const & config):
	requestsMutex(),
	requestAvailable(),
	requests(),
	stopRequested(false),
	stats(),
	config(config),
	getRequestsBuffer()
{
}

RequestsContainer::~RequestsContainer()
{
	DASSERT(this->stopRequested, "requestStop() was not called");
	DASSERT(this->getRequestsBuffer.empty(), "some requests are left in getRequests buffer: " << this->getRequestsBuffer.size());
	DASSERT(this->requests.empty(), "some requests are left in shared buffer: " << this->requests.size());
}

void RequestsContainer::dropAllRequests()
{
	boost::mutex::scoped_lock lock(this->requestsMutex);
	if (!this->requests.empty())
	{
		LOG(WARNING, "dropping " << this->requests.size() << " log requests from shared buffer");
		this->stats.numDropped += this->requests.size();
		this->requests.clear();
	}
	if (!this->getRequestsBuffer.empty())
	{
		LOG(WARNING, "dropping " << this->getRequestsBuffer.size() << " log requests from getRequests buffer");
		this->stats.numDropped += this->getRequestsBuffer.size();
		this->getRequestsBuffer.clear();
	}
}

uint32_t RequestsContainer::getRequests(bool mayBlock, std::string & copyData, uint32_t maxRequests)
{
	// use getRequestsBuffer if possible, otherwise grab some requests under lock
	if (this->getRequestsBuffer.empty())
	{
		DLOG(1, "getRequests: entering mutex");
		boost::mutex::scoped_lock lock(this->requestsMutex);
		if (this->requests.empty() && !mayBlock)
		{
			return 0;
		}
		if (this->requests.empty() && !this->stopRequested)
		{
			++this->stats.numWaitsForRequests;
		}
		while (this->requests.empty() && !this->stopRequested)
		{
			this->requestAvailable.wait(lock);
		}
		if (!this->stopRequested && this->requests.size() < maxRequests)
		{
			// Optimization: there are some requests, but not too many. This means num req/sec is low, so without this optimization
			// this->requests is emptied often and notify_one() in putRequest() is called frequently, which slows down worker threads
			// unnecessarily.
			lock.unlock();
			boost::posix_time::time_duration const sleepTime = boost::posix_time::milliseconds(100);
			DLOG(2, "sleeping " << sleepTime << " waiting for more requests");
			boost::this_thread::sleep(sleepTime);
			lock.lock();
		}
		++this->stats.numMoveToGetRequestsBuffer;
		// moveAllTo is O(1)
		this->requests.moveAllTo(this->getRequestsBuffer, this->getRequestsBuffer.end());
	}
	uint32_t numRequests = 0;
	size_t const initialCapacity = copyData.capacity();
	while (numRequests < maxRequests && !this->getRequestsBuffer.empty())
	{
		// append to std::string
		copyData += this->getRequestsBuffer.front();
		this->getRequestsBuffer.pop_front();
		++numRequests;
	}
	if (copyData.size() > initialCapacity)
	{
		ALERT("size of a postgres COPY buffer for request logging is greater than reserved size",
			copyData.size(), initialCapacity);
	}
	return numRequests;
}

void RequestsContainer::putRequest(std::string const & request)
{
	DASSERT(!request.empty() && request[request.size() - 1] == '\n', "please end me with newline");

	boost::mutex::scoped_lock lock(this->requestsMutex);
	++this->stats.numLogRequests;
	this->stats.numRequestsBytes += request.size();

	// size() is O(1)
	DASSERT(this->requests.size() <= this->config.requestLoggerMaxQueueLength, "queue length exceeded");
	if (this->requests.size() == this->config.requestLoggerMaxQueueLength || this->stopRequested)
	{
		++this->stats.numDropped;
		DLOG(2, "dropping log request", request);
		return;
	}
	DLOG(3, "putRequest", request);

	bool wasEmpty = this->requests.empty();
	this->requests.push_back(request);
	if (wasEmpty)
	{
		this->requestAvailable.notify_one();
	}
}

void RequestsContainer::requestStop()
{
	boost::mutex::scoped_lock lock(this->requestsMutex);
	this->stopRequested = true;
	this->requestAvailable.notify_one();
}

bool RequestsContainer::isStopRequested() const
{
	boost::mutex::scoped_lock lock(this->requestsMutex);
	return this->stopRequested;
}

RequestsStats const & RequestsContainer::getStats() const
{
	boost::mutex::scoped_lock lock(this->requestsMutex);
	return this->stats;
}

/*********************** RequestLoggerThreadStats ************************/

RequestLoggerThreadStats::RequestLoggerThreadStats():
	numConnectionAttempts(0),
	maxWaitForCommitDuration()
{
}

std::ostream & operator<<(std::ostream & out, RequestLoggerThreadStats const & self)
{
	return out << "RequestLoggerThreadStats["
		"numConnectionAttempts: " << self.numConnectionAttempts << ", "
		"maxWaitForCommitDuration: " << self.maxWaitForCommitDuration << "]";
}

/*********************** RequestLogger::ThreadStatsHolder ************************/

void RequestLogger::ThreadStatsHolder::incConnectionAttempts()
{
	boost::mutex::scoped_lock lock(this->threadStatsMutex);
	++this->stats.numConnectionAttempts;
}

void RequestLogger::ThreadStatsHolder::addWaitForCommitDuration(boost::posix_time::time_duration const & duration)
{
	boost::mutex::scoped_lock lock(this->threadStatsMutex);
	this->stats.maxWaitForCommitDuration = std::max(this->stats.maxWaitForCommitDuration, duration);
}

RequestLoggerThreadStats const & RequestLogger::ThreadStatsHolder::getStats() const
{
	boost::mutex::scoped_lock lock(this->threadStatsMutex);
	return this->stats;
}

/**************************** RequestLogger ******************************/

RequestLogger::RequestLogger(Config const & config):
	config(config),
	loggerThread(),
	requestsContainer(this->config),
	statsHolder()
{
	LOG(INFO, "created request logger, max queue length: " << config.requestLoggerMaxQueueLength);
}

void RequestLogger::start()
{
	this->loggerThread.reset(new boost::thread(boost::bind(&RequestLogger::loggerThreadFunction, this)));
}

void RequestLogger::stop()
{
	LOG(INFO, "stopping request logging");
	this->requestsContainer.requestStop();
	this->loggerThread->join();
}

bool RequestLogger::createConnectionPool(std::vector<boost::shared_ptr<db::DBConnection> > & connectionPool) const
{
	// FIXME: make it configurable?
	uint32_t const numConnections = 2;
	for (uint32_t i = 0; i < numConnections; ++i)
	{
		db::DBConnection::ResultType connectionResult = db::DBConnection::create(
			this->config.dbHostLogs,
			this->config.dbPortLogs,
			this->config.dbNameLogs,
			this->config.dbUser,
			this->config.dbPassword,
			this->config.sslMode
		);
		if (connectionResult.isError())
		{
			LOG(ERROR, "unable to connect to logs db: " << connectionResult.getError());
			return false;
		}

		boost::shared_ptr<db::DBConnection> dbConnection = connectionResult.getValue();
		// FIXME: make it configurable?
		dbConnection->setSocketTimeout(boost::posix_time::seconds(20));

		LOG(INFO, "established connection no " << i << " to logs db");
		connectionPool.push_back(dbConnection);
	}
	return true;
}

void RequestLogger::loggerThreadFunction()
{
	LOG(INFO, "request logging thread started");

	bool shouldStop = false;
	while (!shouldStop)
	{
		this->statsHolder.incConnectionAttempts();
		std::vector<boost::shared_ptr<db::DBConnection> > connectionPool;
		if (this->createConnectionPool(connectionPool))
		{
			shouldStop = this->loggingLoop(connectionPool) == STOP_REQUESTED;
		}
		// allow to stop logger in case of db failure
		if (!shouldStop && this->requestsContainer.isStopRequested())
		{
			LOG(INFO, "request logging stop requested while db not available");
			this->requestsContainer.dropAllRequests();
			shouldStop = true;
		}
		if (!shouldStop)
		{
			// sleep to prevent tight loop in case of immediate db failure
			boost::posix_time::time_duration const sleepTime = boost::posix_time::seconds(1);
			LOG(INFO, "sleeping " << sleepTime << " after logs db failure");
			boost::this_thread::sleep(sleepTime);
		}
	}
	LOG(INFO, "request logging thread finished");
}

bool RequestLogger::waitForCommit(db::DBConnection const & dbConnection)
{
	DLOG(1, "waiting for commit");

	boost::posix_time::ptime const startTime = boost::posix_time::microsec_clock::local_time();

	PGresult * result = dbConnection.getResult();
	if (PQresultStatus(result) != PGRES_COMMAND_OK)
	{
		LOG(ERROR, "logs db commit failed: " << PQresStatus(PQresultStatus(result)));
		PQclear(result);
		return false;
	}
	PQclear(result);
	DLOG(1, "requests committed to logs db");

	boost::posix_time::time_duration const duration = boost::posix_time::microsec_clock::local_time() - startTime;
	this->statsHolder.addWaitForCommitDuration(duration);

	return true;
}

RequestLogger::LoggingRetcode RequestLogger::loggingLoop(std::vector<boost::shared_ptr<db::DBConnection> > const & connectionPool)
{
	// FIXME: make it configurable?
	uint32_t const maxRequestsInBatch = 1000;
	uint32_t const maxBatchesInTransaction = 10;

	std::string copyData;
	// FIXME: determine max request size
	copyData.reserve(maxRequestsInBatch * 1000);

	uint32_t currentConnectionNum = 0;

	std::vector<bool> needsCommit(connectionPool.size(), false);

	bool shouldStop = false;
	while (!shouldStop)
	{
		DLOG(1, "using connection: " << currentConnectionNum);
		db::DBConnection const & dbConnection = *connectionPool[currentConnectionNum];
		PGconn * const conn = dbConnection.getHandle();

		if (needsCommit[currentConnectionNum] && !this->waitForCommit(dbConnection))
		{
			return DB_FAILURE;
		}

		DLOG(1, "starting copy");
		// 'id' column has default value
		PGresult * result = dbConnection.execute(
			"copy request_log (server_timestamp, child_timestamp, url, child_uuid, response_code, category_group_id) from stdin");
		if (PQresultStatus(result) != PGRES_COPY_IN)
		{
			LOG(ERROR, "logs db copy start problem: " << PQresStatus(PQresultStatus(result)));
			PQclear(result);
			return DB_FAILURE;
		}
		PQclear(result);

		bool firstRequest = true;
		uint32_t batchNum = 0; // number of non-empty batches
		uint32_t lastBatchSize = maxRequestsInBatch;
		while (batchNum < maxBatchesInTransaction && lastBatchSize == maxRequestsInBatch)
		{
			DLOG(1, "getting requests");
			copyData.clear();
			lastBatchSize = this->requestsContainer.getRequests(firstRequest, copyData, maxRequestsInBatch);
			shouldStop = lastBatchSize == 0 && firstRequest;
			firstRequest = false;
			if (lastBatchSize > 0)
			{
				DLOG(1, "sending batch to logs db", lastBatchSize);
				DLOG(2, "data to be sent: " << copyData);
				if (PQputCopyData(conn, copyData.data(), copyData.size()) != 1)
				{
					LOG(ERROR, "logs db copy data problem", copyData.size());
					return DB_FAILURE;
				}
				DLOG(1, "batch sent to logs db");

				++batchNum;
			}
		}
		DASSERT(batchNum > 0 || shouldStop, "first getRequests() should block");
		DLOG(1, "sent " << batchNum << " batches to logs db");

		if (PQputCopyEnd(conn, NULL) != 1)
		{
			LOG(ERROR, "logs db copy end problem");
			return DB_FAILURE;
		}
		DLOG(1, "after copy end");

		needsCommit[currentConnectionNum] = true;

		currentConnectionNum = (currentConnectionNum + 1) % connectionPool.size();
	}
	// stop was requested, wait for commit of all pending transactions
	for (uint32_t i = 0; i < connectionPool.size(); ++i)
	{
		db::DBConnection const & dbConnection = *connectionPool[i];
		if (needsCommit[i] && !this->waitForCommit(dbConnection))
		{
			break;
		}
	}
	return STOP_REQUESTED;
}

/*
 * Performs 3 kinds of escaping:
 * - for the need of COPY FROM STDIN: some characters (tab, newline, backslash) have special meaning
 * - to comply with db charset: it is utf-8, but in fact we use only ascii subset
 * - remove url content after '?' to reduce log db usage
 */
static std::string escapeString(std::string const & src)
{
	std::string ret;
	// At most two characters per input character.
	// Normally push_back() reserves space for just one character.
	ret.reserve(2 * src.size());

	//Question mark can be encoded in url as %3F
	int questionMarkFirstCharPosition = -1;
	int questionMarkSecondCharPosition = -1;

	int i = 0;
	for (std::string::const_iterator it = src.begin(); it != src.end(); ++it)
	{
		unsigned char const chr = *it;
		switch (chr)
		{
		case '%':
			questionMarkFirstCharPosition = i;
			ret.push_back(chr);
			break;
		case '3':
			questionMarkSecondCharPosition = i;
			ret.push_back(chr);
			break;
		case 'F':
			if(questionMarkFirstCharPosition == i-2 && questionMarkSecondCharPosition == i-1)
			{
				ret.push_back(chr);
				return ret;
			}
			ret.push_back(chr);
			break;
		case '?':
			ret.push_back(chr);
			return ret;
		case '\n':
			ret.append("\\n");
			break;
		case '\t':
			ret.append("\\t");
			break;
		case '\\':
			ret.append("\\\\"); // double backslash
			break;
		default:
			if (chr < 128)
			{
				ret.push_back(chr);
			}
			else
			{
				ret.push_back('?');
			}
			break;
		}
		++i;
	}

	return ret;
}

/*
 * requestUrl is assumed to be ascii-only and is logged as-is. Characters outside of ascii are
 * replaced with '?'.
 */
void RequestLogger::logRequest(
	std::string const & requestUrl,
	db::Uuid const & childUuid,
	BackendQueryResult const & result
)
{
	DASSERT(!result.utcTime.is_special() && !result.childTime.is_special(), "time must be set");

	std::string const escapedUrl = escapeString(requestUrl);

	ASSERT(escapedUrl.find_first_of("\t\n") == std::string::npos,
		"we do not allow special characters in url (tab, newline)", escapedUrl);

	std::string const serverTimeStr = boost::posix_time::to_simple_string(result.utcTime);
	std::string const childTimeStr = boost::posix_time::to_simple_string(result.childTime);

	std::string const null("\\N");
	std::ostringstream sStream;
	sStream
		// server_timestamp timestamp not null,
		<< serverTimeStr << "\t"
		// child_timestamp timestamp not null,
		<< childTimeStr << "\t"
		// url text not null,
		<< escapedUrl << "\t"
		// child_uuid text not null,
		<< childUuid << "\t"
		// response_code integer not null,
		<< *result.responseCode << "\t"
		// category_group_id integer
		<< (result.categoryGroupInfo ? boost::lexical_cast<std::string>(result.categoryGroupInfo->first) : null) << "\n";

	this->requestsContainer.putRequest(sStream.str());
}

RequestsStats const & RequestLogger::getRequestsStats() const
{
	return this->requestsContainer.getStats();
}

RequestLoggerThreadStats const & RequestLogger::getRequestLoggerThreadStats() const
{
	return this->statsHolder.getStats();
}

} // namespace backend
} // namespace safekiddo

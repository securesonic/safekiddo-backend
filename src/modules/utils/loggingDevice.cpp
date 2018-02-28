#include <syscall.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>

#include "utils/loggingDevice.h"
#include "utils/assert.h"
#include "utils/foreach.h"


namespace safekiddo
{
namespace utils
{

pid_t LoggingDevice::getTid() const
{
	return syscall(SYS_gettid);
}

LoggingDevice::LoggingDevice(LogLevelEnum logLevel):
	logMutex(),
	logLevel(logLevel)
{
}

LoggingDevice::~LoggingDevice()
{
}

bool LoggingDevice::checkLoggingLevel(LogLevelEnum const logLevel) const
{
	boost::mutex::scoped_lock mainLock(this->logMutex);
	return logLevel <= this->logLevel;
}

void LoggingDevice::setLoggingLevel(LogLevelEnum logLevel)
{
	boost::mutex::scoped_lock mainLock(this->logMutex);
	this->logLevel = logLevel;
}

void LoggingDevice::writeLog(
	LogLevelEnum severity,
	std::string const & file,
	uint32_t line,
	std::string const & function,
	std::string const & message
)
{
	std::ostringstream oss;
	this->formatLog(oss, severity, file, line, function, message);
	this->outputLog(severity, oss.str());
}

void LoggingDevice::formatLog(
	std::ostringstream & oss,
	LogLevelEnum severity,
	std::string const & file,
	uint32_t line,
	std::string const & /*function*/,
	std::string const & message
)
{
	boost::posix_time::ptime localTime = boost::posix_time::microsec_clock::local_time();
	std::string timeStr = boost::posix_time::to_iso_extended_string(localTime);
	if (timeStr.size() > 10)
	{
		timeStr[10] = ' '; // change date-time separator from 'T' to ' '
		if (timeStr.size() > 20) // 1970-01-01 00:00:00.000001
		{
			// truncate microseconds from the time representation. Only milliseconds are interesting.
			timeStr.erase(timeStr.size() - 3);
		}
		else
		{
			// microseconds were skipped; pad with zeros for good alignment.
			timeStr = timeStr + ".000";
		}
	}

	oss << "[" << timeStr << "] " << this->getStringSeverity(severity);

	oss << message;
	using namespace LogLevel;
    if (severity == DEBUG1 ||  severity == DEBUG2 || severity == DEBUG3)
    {
	   oss << " <" << this->getTid() << ">" << " (" << file << ":" << line << ")";
    }
    oss << "\n";
    
}

// static variables without initializer are initialized to zero by dynamic linker
LoggingDevice * LoggingDevice::impl;

void LoggingDevice::setImpl(LoggingDevice * impl)
{
	LoggingDevice::impl = impl;
}

LoggingDevice * LoggingDevice::getImpl()
{
	return LoggingDevice::impl;
}

std::string LoggingDevice::getStringSeverity(LogLevelEnum const severity) const
{
	using namespace LogLevel;
	switch (severity)
	{
	case NONE:
		FAILURE("NONE log level is not valid for log output");
	case FATAL:
		return "FATAL ";
	case ERROR:
		return "ERROR ";
	case WARNING:
		return "WARN  ";
	case INFO:
		return "INFO  ";
	case DEBUG1:
		return "DEBUG1";
	case DEBUG2:
		return "DEBUG2";
	case DEBUG3:
		return "DEBUG3";
	}
	FAILURE("unknown log level: " << severity);
}

#define MATCH_LOG_LEVEL(str, code)\
	case code: \
		if (str == # code) \
		{ \
			return code; \
		} \
		// no break!

boost::optional<LogLevelEnum> LoggingDevice::parseLogLevel(std::string const & str)
{
	using namespace LogLevel;
	// this code will generate a compiler warning if any enum value is not handled in the switch.

	LogLevelEnum dummy = NONE; // take the first one (important!), no 'const' here

	switch (dummy)
	{
	MATCH_LOG_LEVEL(str, NONE);
	MATCH_LOG_LEVEL(str, FATAL);
	MATCH_LOG_LEVEL(str, ERROR);
	MATCH_LOG_LEVEL(str, WARNING);
	MATCH_LOG_LEVEL(str, INFO);
	MATCH_LOG_LEVEL(str, DEBUG1);
	MATCH_LOG_LEVEL(str, DEBUG2);
	MATCH_LOG_LEVEL(str, DEBUG3);
	break;
	}
	return boost::none;
}

#undef MATCH_LOG_LEVEL

StdLoggingDevice::StdLoggingDevice(LogLevelEnum logLevel, std::ostream & outStream):
	LoggingDevice(logLevel),
	flushMutex(),
	condition(),
	logEntries(),
	stopRequested(false),
	flushThread(),
	outStream(outStream)
{
}

void StdLoggingDevice::start()
{
	this->flushThread.reset(new boost::thread(boost::bind(&StdLoggingDevice::threadFunction, this)));
}

void StdLoggingDevice::stop()
{
	DLOG(1, "stopping logger thread");
	boost::mutex::scoped_lock mainLock(this->logMutex);
	this->stopRequested = true;
	this->condition.notify_one();
	mainLock.unlock();
	this->flushThread->join();
	DASSERT(this->logEntries.empty(), "some log entries are left in shared buffer (" << this->logEntries.size() <<
		"), first is: " << this->logEntries.front().buf);
}

void StdLoggingDevice::threadFunction()
{
	boost::mutex::scoped_lock mainLock(this->logMutex);

	// Second part of below condition is important: in flushLogEntries() mainLock is unlocked so
	// there may come more logs and we want to call flushLogEntries() again even if stopRequested is
	// true.
	while (!(this->stopRequested && this->logEntries.empty()))
	{
		while (!this->stopRequested && this->logEntries.empty())
		{
			this->condition.wait(mainLock);
		}
		if (!this->logEntries.empty())
		{
			this->flushLogEntries(mainLock);
		}
	}
}

void StdLoggingDevice::flushLogEntries(boost::mutex::scoped_lock & mainLock)
{
	std::list<LogEntry> flushList;
	flushList.swap(this->logEntries);
	// Don't return here if flushList is empty: if we are called from flush() then it must take flushLock
	// to ensure the bg thread has finished operating on outStream.

	boost::mutex::scoped_lock flushLock(this->flushMutex);
	// Unlock it here because flush() can be called from any thread.
	mainLock.unlock();

	// No need to do a flush on outStream if there are no entries to write because anything that was
	// written earlier has already been flushed.
	if (!flushList.empty())
	{
		bool const wasBad = this->outStream.bad();
		// clear stream state in case there was out-of-space and has already ended
		this->outStream.clear();
		FOREACH_CREF(logEntry, flushList)
		{
			this->outStream << logEntry.buf;
			if (logEntry.logLevel == LogLevel::FATAL || logEntry.logLevel == LogLevel::ERROR)
			{
				std::cerr << logEntry.buf;
			}
		}
		this->outStream.flush();
		bool const isBad = this->outStream.bad();
		if (!wasBad && isBad)
		{
			std::cerr << "StdLoggingDevice: log device failed" << std::endl;
		}
		else if (wasBad && !isBad)
		{
			std::cerr << "StdLoggingDevice: log device recovered" << std::endl;
		}
	}

	flushLock.unlock();
	// The two lines must not be swapped to respect correct lock order:
	// mainLock -> flushLock
	mainLock.lock();
}

void StdLoggingDevice::flush()
{
	boost::mutex::scoped_lock mainLock(this->logMutex);
	this->flushLogEntries(mainLock);
}

void StdLoggingDevice::reopen()
{
	// does not make sense
	FAILURE("not implemented");
}

void StdLoggingDevice::outputLog(LogLevelEnum const logLevel, std::string const & buf)
{
	boost::mutex::scoped_lock mainLock(this->logMutex);
	bool const wasEmpty = this->logEntries.empty();
	this->logEntries.push_back(LogEntry(logLevel, buf));
	if (wasEmpty)
	{
		this->condition.notify_one();
	}
}

StdLoggingDevice::LogEntry::LogEntry(
	LogLevelEnum logLevel,
	std::string const & buf
):
	logLevel(logLevel),
	buf(buf)
{
}

FileLoggingDevice::FileLoggingDevice(
	std::string const & logFilePath,
	LogLevelEnum logLevel
):
	StdLoggingDevice(logLevel, this->logFile),
	logFile(),
	logFilePath(logFilePath)
{
	this->openLogFile();
}

FileLoggingDevice::~FileLoggingDevice()
{
	this->closeLogFile();
}

void FileLoggingDevice::reopen()
{
	// Note we are operating directly on the stream we passed to StdLoggingDevice which protects it
	// by flushMutex.
	boost::mutex::scoped_lock flushLock(this->flushMutex);

	this->closeLogFile();
	this->openLogFile();
}

void FileLoggingDevice::openLogFile()
{
	this->logFile.clear();
	this->logFile.open(this->logFilePath.c_str(), std::ios::app);
	// FIXME: SAF-294: backend could work without logs
	// Also possible to move log opening inside StdLoggingDevice::flushLogEntries() as it
	// already is responsible for clearing log device failures.
	ASSERT(this->logFile.is_open(), "cannot open log file " << this->logFilePath);
	this->logFile << "--- LOG STARTS ---\n";
}

void FileLoggingDevice::closeLogFile()
{
	this->logFile << "--- LOG ENDS ---\n";
	this->logFile.close();
}

ScopedLoggingDeviceSetter::ScopedLoggingDeviceSetter(LoggingDevice * impl):
	oldImpl(LoggingDevice::getImpl())
{
	LoggingDevice::setImpl(impl);
	impl->start();
}

ScopedLoggingDeviceSetter::~ScopedLoggingDeviceSetter()
{
	LoggingDevice::getImpl()->stop();
	LoggingDevice::setImpl(this->oldImpl);
}

} // utils
} // safekiddo

#ifndef _SAFEKIDDO_UTILS_LOGGING_DEVICE_H
#define _SAFEKIDDO_UTILS_LOGGING_DEVICE_H

#include <sys/types.h>
#include <string>
#include <fstream>
#include <list>

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/optional.hpp>

#include "logMacro.h"

namespace safekiddo
{
namespace utils
{

class LoggingDevice
{
public:
	virtual ~LoggingDevice();

	bool checkLoggingLevel(LogLevelEnum logLevel) const;

	void setLoggingLevel(LogLevelEnum logLevel);

	// may be synchronous or asynchronous depending on subclass
	void writeLog(
		LogLevelEnum severity,
		std::string const & file,
		uint32_t line,
		std::string const & function,
		std::string const & message
	);

	virtual void start() = 0;
	virtual void stop() = 0;

	// synchronously flushes any buffer used by the LoggingDevice
	// does NOT force data to be synced to a physical device
	virtual void flush() = 0;

	// closes and opens the log again; might not make sense for every subclass
	virtual void reopen() = 0;

	static void setImpl(LoggingDevice *); // not thread-safe
	static LoggingDevice * getImpl();

	static boost::optional<LogLevelEnum> parseLogLevel(std::string const & str);

protected:
	LoggingDevice(LogLevelEnum logLevel);

	virtual void outputLog(LogLevelEnum const severity, std::string const & buf) = 0;


	boost::mutex mutable logMutex;

private:
	void formatLog(
		std::ostringstream & oss,
		LogLevelEnum severity,
		std::string const & file,
		uint32_t line,
		std::string const & function,
		std::string const & message
	);

	pid_t getTid() const;

	std::string getStringSeverity(LogLevelEnum const severity) const;


	LogLevelEnum logLevel;

	static LoggingDevice * impl;
};

class StdLoggingDevice: public LoggingDevice
{
public:
	// don't use 'outStream' in constructor, it is not initialized at this point
	StdLoggingDevice(LogLevelEnum logLevel, std::ostream & outStream);

	virtual void start();
	virtual void stop();

	virtual void flush();

	virtual void reopen();

protected:
	// asynchronous (and so writeLog is)
	virtual void outputLog(LogLevelEnum const severity, std::string const & buf);

	boost::mutex mutable flushMutex;

private:
	struct LogEntry
	{
		LogLevelEnum logLevel;
		std::string buf;

		LogEntry(
			LogLevelEnum logLevel,
			std::string const & buf
		);
	};

	void threadFunction();

	void flushLogEntries(boost::mutex::scoped_lock & mainLock);

	// used with logMutex
	boost::condition_variable mutable condition;
	std::list<LogEntry> logEntries;
	bool stopRequested;

	boost::scoped_ptr<boost::thread> flushThread;

	// accessed under flushMutex
	std::ostream & outStream;
};

class FileLoggingDevice: public StdLoggingDevice
{
public:
	FileLoggingDevice(
		std::string const & logFilePath,
		LogLevelEnum logLevel
	);

	virtual ~FileLoggingDevice();

	virtual void reopen();

private:
	void openLogFile();

	void closeLogFile();


	std::ofstream logFile;

	std::string const logFilePath;
};

class ScopedLoggingDeviceSetter
{
public:
	ScopedLoggingDeviceSetter(LoggingDevice * impl);

	~ScopedLoggingDeviceSetter();

private:
	LoggingDevice * oldImpl;
};

} // utils
} // safekiddo

#endif

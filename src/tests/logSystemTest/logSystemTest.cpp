#include "utils/loggingDevice.h"
#include "utils/rawAssert.h"
#include "utils/timeUtils.h"

#include <stdint.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <ext/stdio_filebuf.h>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

namespace su = safekiddo::utils;

namespace safekiddo
{
namespace backend
{
namespace testing
{

void readLines(std::string const & path, std::vector<std::string> & lines)
{
	std::ifstream inp(path.c_str());
	RAW_ASSERT(inp, "cannot open file " << path);
	std::string line;
	while (std::getline(inp, line))
	{
		lines.push_back(line);
	}
}

void removeFile(std::string const & path)
{
	int ret = ::unlink(path.c_str());
	int const err = errno;
	RAW_ASSERT(ret == 0, "cannot unlink " << path << ", errno " << err);
}

size_t getFileSize(std::string const & path)
{
	struct stat st;
	int ret = ::stat(path.c_str(), &st);
	int const err = errno;
	RAW_ASSERT(ret == 0, "cannot stat " << path << ", errno " << err);
	return st.st_size;
}

/*
 * Test steps:
 * - start log system
 * - call log macro a few times
 * - stop log system
 * - open the log file and read contents
 * - first line must contain "LOG STARTS"
 * - last line must contain "LOG ENDS"
 * - check if logged lines are there and in order
 * - remove log file
 */
void basicLoggingTest()
{
	std::string const logFilePath("test.log");
	int32_t const numLogLines = 10;

	{
		su::FileLoggingDevice loggingDevice(logFilePath, su::LogLevel::INFO);
		su::ScopedLoggingDeviceSetter logSetter(&loggingDevice);

		for (int32_t i = 0; i < numLogLines; ++i)
		{
			LOG(INFO, "this is log line no " << i);
		}
	}

	std::vector<std::string> logLines;
	readLines(logFilePath, logLines);

	std::vector<std::string>::iterator it = logLines.begin();

	RAW_ASSERT(it != logLines.end() && it->find("LOG STARTS") != std::string::npos, "missing log start");
	++it;

	for (int32_t i = 0; i < numLogLines; ++i)
	{
		std::ostringstream oss;
		oss << "this is log line no " << i;
		RAW_ASSERT(it != logLines.end() && it->find(oss.str()) != std::string::npos,
			"unexpected log line '" << *it << "' instead of expected '" << oss.str() << "'");
		++it;
	}

	RAW_ASSERT(it != logLines.end() && it->find("LOG ENDS") != std::string::npos, "missing log end");
	++it;

	RAW_ASSERT(it == logLines.end(), "unexpected line after log end: " << *it);

	removeFile(logFilePath);
}

/*
 * Test steps:
 * - start log system
 * - log one line
 * - stop log system
 * - repeat previous steps once again
 * - verify log file contents
 * - remove log file
 */
void appendTest()
{
	std::string const logFilePath("test.log");
	for (int32_t i = 0; i < 2; ++i)
	{
		su::FileLoggingDevice loggingDevice(logFilePath, su::LogLevel::INFO);
		su::ScopedLoggingDeviceSetter logSetter(&loggingDevice);

		LOG(INFO, "log file incarnation " << i);
	}
	std::vector<std::string> logLines;
	readLines(logFilePath, logLines);

	std::vector<std::string>::iterator it = logLines.begin();

	for (int32_t i = 0; i < 2; ++i)
	{
		RAW_ASSERT(it != logLines.end() && it->find("LOG STARTS") != std::string::npos, "missing log start");
		++it;

		std::ostringstream oss;
		oss << "log file incarnation " << i;
		RAW_ASSERT(it != logLines.end() && it->find(oss.str()) != std::string::npos,
			"unexpected log line '" << *it << "' instead of expected '" << oss.str() << "'");
		++it;

		RAW_ASSERT(it != logLines.end() && it->find("LOG ENDS") != std::string::npos, "missing log end");
		++it;
	}

	RAW_ASSERT(it == logLines.end(), "unexpected line after log end: " << *it);

	removeFile(logFilePath);
}

/*
 * A reading thread with some hardcoded delay.
 */
class SlowBackgroundReader
{
public:
	SlowBackgroundReader(
		std::istream & inStream,
		int numToRead
	):
		inStream(inStream),
		numToRead(numToRead),
		thread(),
		mutex(),
		numLinesRead(0)
	{
		this->thread.reset(new boost::thread(boost::bind(&SlowBackgroundReader::threadFunction, this)));
	}

	void join()
	{
		this->thread->join();
	}

	int getNumLinesRead() const
	{
		boost::mutex::scoped_lock lock(this->mutex);
		return this->numLinesRead;
	}

private:
	void threadFunction()
	{
		boost::posix_time::ptime const startTime = boost::posix_time::microsec_clock::local_time();
		for (int i = 0; i < this->numToRead; ++i)
		{
			if (i % 10 == 0)
			{
				boost::posix_time::time_duration const duration = boost::posix_time::microsec_clock::local_time() - startTime;
				// all reading should take about 1s
				boost::posix_time::time_duration const expectedDuration = boost::posix_time::milliseconds(1000.0 * i / this->numToRead);
				if (duration < expectedDuration)
				{
					boost::posix_time::time_duration const sleepTime = expectedDuration - duration;
					boost::this_thread::sleep(sleepTime);
				}
			}
			std::string line;
			std::getline(this->inStream, line);
			boost::mutex::scoped_lock lock(this->mutex);
			++this->numLinesRead;
		}
	}

	std::istream & inStream;
	int numToRead;

	boost::scoped_ptr<boost::thread> thread;

	boost::mutex mutable mutex;
	int numLinesRead;
};

/*
 * Test steps:
 * - create an in-process pipe
 * - start log system that writes to the pipe
 * - log 10k lines ~100 bytes each (data is buffered inside logging system
 *   and pipe read is not necessary - a check that logging device temporary slowness
 *   does not block backend); note: pipe kernel buffer is supposed to be 64kb
 * - start a background thread which consumes input of the pipe with speed limit so
 *   that whole operation takes about 1s
 * - immediately after starting thread check how many lines it has read, not all
 *   should be
 * - flush log system
 * - join background thread (flush only ensured that all data was written to pipe, but
 *   some of it may still reside in pipe kernel buffer)
 * - check the number of lines consumed by background thread, all must be read
 * - check that flush took no less than 1s
 * - check that join was quick
 * - stop log system
 * - destroy pipe
 */
void asyncLogTest()
{
	int pipefd[2];
	int ret = ::pipe(pipefd);
	RAW_ASSERT(ret == 0, "pipe() failed");

	// descriptors are closed on destruction
	__gnu_cxx::stdio_filebuf<char> inBuf(pipefd[0], std::ios::in);
	__gnu_cxx::stdio_filebuf<char> outBuf(pipefd[1], std::ios::out);

	std::istream inStream(&inBuf);
	std::ostream outStream(&outBuf);

	su::StdLoggingDevice loggingDevice(su::LogLevel::INFO, outStream);
	su::ScopedLoggingDeviceSetter logSetter(&loggingDevice);

	std::string const logLine(30, 'X'); // with overheads makes about 100-byte log line
	int const numLogLines = 10000;
	for (int i = 0; i < numLogLines; ++i)
	{
		LOG(INFO, logLine << i);
	}

	// StdLoggingDevice does not add "LOG STARTS" line
	int const numToRead = numLogLines;
	SlowBackgroundReader slowBackgroundReader(inStream, numToRead);

	int numLinesRead = slowBackgroundReader.getNumLinesRead();
	RAW_ASSERT(numLinesRead < numToRead, "receiving of log lines should have some delay");

	BEGIN_TIMED_BLOCK(flush)
	// waits until log thread writes all log to the pipe
	loggingDevice.flush();
	END_TIMED_BLOCK(flush)

	BEGIN_TIMED_BLOCK(join)
	// background reader may still need to read remaining 64kb of data from pipe's kernel buffer
	slowBackgroundReader.join();
	END_TIMED_BLOCK(join)

	numLinesRead = slowBackgroundReader.getNumLinesRead();
	RAW_ASSERT(numLinesRead == numToRead, "log system was not flushed");

	// Flush may be shorter than 1s due to the 64kb pipe's kernel buffer.
	RAW_ASSERT(flushDuration >= boost::posix_time::milliseconds(900), "flush should take about 1s, actual duration: "
		<< flushDuration);
	RAW_ASSERT(flushDuration < boost::posix_time::milliseconds(1500), "flush should not be much slower than reader thread, "
		"flush duration: " << flushDuration);
	RAW_ASSERT(joinDuration < boost::posix_time::milliseconds(200), "join should be much shorter than flush, actual duration: "
		<< joinDuration);
}

/*
 * Test steps:
 * - create a directory and mount in it a tmpfs of size 1MB
 * - start log system with log file on tmpfs
 * - loop until log file size stops growing:
 *   - write 1k lines ~100 bytes each
 *   - flush log
 * - verify log file size is at least 95% of tmpfs size
 * - remount tmpfs to a greater size
 * - sleep 1s
 * - verify log file size is still the same
 * - repeat 2 times (after first round some garbage from stream may be written):
 *   - log one line
 *   - sleep 1s
 *   - check that log file size is bigger
 * - stop log system
 * - unmount tmpfs and remove the mountpoint directory
 */
void diskFullTest()
{
	int ret = ::system("mkdir -p tmpfsmnt && sudo mount -t tmpfs -o size=1m dummy tmpfsmnt");
	RAW_ASSERT(ret == 0, "cannot mount tmpfs");

	std::string const logFilePath("tmpfsmnt/test.log");

	{
		su::FileLoggingDevice loggingDevice(logFilePath, su::LogLevel::INFO);
		su::ScopedLoggingDeviceSetter logSetter(&loggingDevice);

		size_t logFileSize = 0;
		while (true)
		{
			std::string const logLine(30, 'X'); // with overheads makes about 100-byte log line
			for (int i = 0; i < 1000; ++i)
			{
				LOG(INFO, logLine << i);
			}
			loggingDevice.flush();

			size_t const newFileSize = getFileSize(logFilePath);
			if (newFileSize == logFileSize)
			{
				break;
			}
			logFileSize = newFileSize;
		}

		RAW_ASSERT(logFileSize >= 1024 * 1024 * 95 / 100, "log file does not fill tmpfs, size: " << logFileSize);

		ret = ::system("sudo mount -t tmpfs -o remount,size=2m dummy tmpfsmnt");
		RAW_ASSERT(ret == 0, "cannot remount tmpfs");

		boost::this_thread::sleep(boost::posix_time::seconds(1));

		RAW_ASSERT(getFileSize(logFilePath) == logFileSize, "some new logs written after remount?");

		for (int i = 0; i < 2; ++i)
		{
			LOG(INFO, "a log line no " << i);

			boost::this_thread::sleep(boost::posix_time::seconds(1));

			size_t const newFileSize = getFileSize(logFilePath);
			RAW_ASSERT(newFileSize > logFileSize, "log writing not resumed after out-of-space ended");
			logFileSize = newFileSize;
		}
	}

	ret = ::system("sudo umount tmpfsmnt && rmdir tmpfsmnt");
	RAW_ASSERT(ret == 0, "cannot unmount tmpfs");
}

#define RUN_TEST(name) \
	std::cout << "Running test: " # name << std::endl; \
	name()

void runTests()
{
	RUN_TEST(basicLoggingTest);
	RUN_TEST(appendTest);
	RUN_TEST(asyncLogTest);
	RUN_TEST(diskFullTest);
}

} // namespace testing
} // namespace backend
} // namespace safekiddo

int32_t main(int32_t const argc, char ** argv)
{
	safekiddo::backend::testing::runTests();
	return 0;
}

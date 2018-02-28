#include "utils/management.h"
#include "utils/assert.h"
#include "utils/loggingDevice.h"
#include "utils/rawAssert.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace safekiddo
{
namespace utils
{

MgmtInterface::MgmtInterface(
	std::string const & mgmtFifoPath
):
	mgmtFifoPath(mgmtFifoPath),
	mgmtThread(),
	fifoFile(),
	stopRequested(false)
{
	this->ensureFifoExists();
	// 'out' mode is needed to avoid getting an EOF after user echoes a command to the fifo
	this->fifoFile.open(this->mgmtFifoPath.c_str(), std::ios::in | std::ios::out);
	ASSERT(this->fifoFile.is_open(), "cannot open management fifo file " << this->mgmtFifoPath);
}

MgmtInterface::~MgmtInterface()
{
	this->mgmtThread->join();
}

void MgmtInterface::start()
{
	this->mgmtThread.reset(new boost::thread(boost::bind(&MgmtInterface::threadFunction, this)));
}

void MgmtInterface::requestStop()
{
	this->stopRequested = true;
}

void MgmtInterface::threadFunction()
{
	LOG(INFO, "management thread started");
	while (!this->stopRequested)
	{
		std::string line;
		std::getline(this->fifoFile, line);
		ASSERT(this->fifoFile, "management fifo read error");
		LOG(INFO, "MgmtInterface: executing '" << line << "'");
		this->processCommand(line);
	}
	LOG(INFO, "management thread finished");
}

void MgmtInterface::ensureFifoExists()
{
	struct stat st;
	if (::stat(this->mgmtFifoPath.c_str(), &st) == -1)
	{
		int err = errno;
		LOG(INFO, "cannot stat management fifo file", err);
		if (err != ENOENT)
		{
			FAILURE("unknown error from stat()", err);
		}
		if (::mkfifo(this->mgmtFifoPath.c_str(), 0600) == -1)
		{
			int err = errno;
			FAILURE("cannot create management fifo file", err);
		}
	}
	else
	{
		ASSERT(S_ISFIFO(st.st_mode), "file is not a fifo: " << this->mgmtFifoPath);
	}
}

void MgmtInterface::processCommand(std::string const & line)
{
	std::istringstream stream(line);
	std::string cmdName;
	stream >> cmdName;

	if (this->handleCommand(cmdName, stream))
	{
		// empty - command handled in a subclass
	}
	else if (cmdName == "flushLogs")
	{
		this->cmdFlushLogs(stream);
	}
	else if (cmdName == "setLogLevel")
	{
		this->cmdSetLogLevel(stream);
	}
	else if (cmdName == "reopenLog")
	{
		this->cmdReopenLog(stream);
	}
	else
	{
		LOG(ERROR, "MgmtInterface: unknown command '" << cmdName << "'");
	}

	if (!stream)
	{
		LOG(ERROR, "MgmtInterface: invalid arguments in command '" << line << "'");
	}
}

bool MgmtInterface::handleCommand(std::string const &, std::istringstream &)
{
	return false;
}

void MgmtInterface::cmdFlushLogs(std::istringstream &)
{
	LoggingDevice * ptr = LoggingDevice::getImpl();
	RAW_DASSERT(ptr != NULL, "logging system not initialized");

	ptr->flush();
}

void MgmtInterface::cmdSetLogLevel(std::istringstream & args)
{
	std::string logLevel;
	args >> logLevel;
	boost::optional<LogLevelEnum> logLevelEnum = LoggingDevice::parseLogLevel(logLevel);
	if (!logLevelEnum)
	{
		LOG(ERROR, "Invalid logLevel argument: " << logLevel);
		return;
	}
	LoggingDevice * ptr = LoggingDevice::getImpl();
	RAW_DASSERT(ptr != NULL, "logging system not initialized");

	ptr->setLoggingLevel(logLevelEnum.get());
}

void MgmtInterface::cmdReopenLog(std::istringstream &)
{
	LoggingDevice * ptr = LoggingDevice::getImpl();
	RAW_DASSERT(ptr != NULL, "logging system not initialized");

	ptr->reopen();
}

} // namespace utils
} // namespace safekiddo

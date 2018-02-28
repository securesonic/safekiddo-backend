#include "utils/loggingDevice.h"
#include "utils/rawAssert.h"

namespace safekiddo
{
namespace utils
{

bool checkLogLevel(LogLevelEnum level)
{
	LoggingDevice * ptr = LoggingDevice::getImpl();
	RAW_DASSERT(ptr != NULL, "logging system not initialized");
	return ptr && ptr->checkLoggingLevel(level);
}

void handleLogLine(LogLevelEnum level, char const * file, uint32_t line, char const * function, char const * msg)
{
	LoggingDevice * ptr = LoggingDevice::getImpl();
	RAW_DASSERT(ptr != NULL, "logging system not initialized");
	if (ptr)
	{
		ptr->writeLog(level, file, line, function, msg);
	}
}

void flushLogBuffers()
{
	LoggingDevice * ptr = LoggingDevice::getImpl();
	RAW_DASSERT(ptr != NULL, "logging system not initialized");
	if (ptr)
	{
		ptr->flush();
	}
}

} // utils
} // safekiddo

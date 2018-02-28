#ifndef _SAFEKIDDO_UTILS_LOG_MACRO_H
#define _SAFEKIDDO_UTILS_LOG_MACRO_H

#include <stdint.h>
#include <sstream>
#include <boost/current_function.hpp>

#include "macros.h"

#define LOG_IGNORE(streamableCode, ...) \
	{\
		if (0) \
		{\
			std::ostringstream oss; \
			oss << streamableCode << CONVERT_ARGS_TO_STREAM(__VA_ARGS__); \
		}\
	}

#define LOG_IMPL(level, file, line, streamableCode, ...) \
	{\
		using namespace safekiddo::utils::LogLevel; \
		if (safekiddo::utils::checkLogLevel(level)) \
		{\
			std::ostringstream oss; \
			oss << streamableCode << CONVERT_ARGS_TO_STREAM(__VA_ARGS__); \
			safekiddo::utils::handleLogLine(level, file, line, BOOST_CURRENT_FUNCTION, oss.str().c_str()); \
		}\
	}

#define LOG(level, streamableCode, ...) LOG_IMPL(level, __FILE__, __LINE__, streamableCode, ## __VA_ARGS__)

// Compile DLOGs also in release config to allow for runtime log level switching.
/*#ifdef NDEBUG

#define DLOG(level, streamableCode, ...) LOG_IGNORE(streamableCode, ## __VA_ARGS__)

#else*/

#define MAP_DEBUG_LEVEL(level) static_cast<safekiddo::utils::LogLevelEnum>((level) - 1 + safekiddo::utils::LogLevel::DEBUG1)

#define DLOG(level, streamableCode, ...) \
	{\
		safekiddo::utils::LogLevelEnum mapped = MAP_DEBUG_LEVEL(level); \
		LOG(mapped, streamableCode, ## __VA_ARGS__); \
	}

//#endif // NDEBUG

namespace safekiddo
{
namespace utils
{

namespace LogLevel
{

enum LogLevel
{
	NONE = 0, // no logging
	FATAL = 1,
	ERROR = 2,
	WARNING = 3,
	INFO = 4,
	DEBUG1 = 5,
	DEBUG2 = 6,
	DEBUG3 = 7,
};

} // namespace LogLevel

typedef LogLevel::LogLevel LogLevelEnum;

bool checkLogLevel(LogLevelEnum);
void handleLogLine(LogLevelEnum, char const * file, uint32_t line, char const * function, char const * msg);
void flushLogBuffers();

} // utils
} // safekiddo

#endif

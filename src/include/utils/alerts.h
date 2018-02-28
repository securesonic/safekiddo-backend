#ifndef _SAFEKIDDO_UTILS_ALERTS_H
#define _SAFEKIDDO_UTILS_ALERTS_H

#include "utils/logMacro.h"
#include "utils/backtrace.h"

namespace safekiddo
{

#define ALERT_PROD(streamableCode, ...) \
	do { \
		LOG(ERROR, "Unexpected event with message: <" << streamableCode << ">", ## __VA_ARGS__); \
		LOG(FATAL, "Backtrace: " << safekiddo::utils::backtrace()); \
		safekiddo::utils::flushLogBuffers(); \
	} while(0)

#define ALERT_DEBUG(streamableCode, ...) \
	do { \
		LOG(FATAL, "Unexpected event with message: <" << streamableCode << ">", ## __VA_ARGS__); \
		LOG(FATAL, "Backtrace: " << safekiddo::utils::backtrace()); \
		safekiddo::utils::flushLogBuffers(); \
		::abort(); \
	} while(0)

#ifdef NDEBUG

#define ALERT(...) ALERT_PROD(__VA_ARGS__)

#else

#define ALERT(...) ALERT_DEBUG(__VA_ARGS__)

#endif // NDEBUG


}

#endif // _SAFEKIDDO_UTILS_ALERTS_H

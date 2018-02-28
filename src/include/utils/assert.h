#ifndef _SAFEKIDDO_UTILS_ASSERT_H
#define _SAFEKIDDO_UTILS_ASSERT_H

#include <stdlib.h>
#include <errno.h>

#include "logMacro.h"
#include "backtrace.h"

#define ASSERT(cond, streamableCode, ...) \
	do { \
		if (!(cond)) \
		{\
			LOG(FATAL, "Assertion failed: <" << #cond << ">, with message: <" << streamableCode << ">", ## __VA_ARGS__); \
			LOG(FATAL, "Backtrace: " << safekiddo::utils::backtrace()); \
			safekiddo::utils::flushLogBuffers(); \
			::abort(); \
		}\
	} while(0)

#ifdef NDEBUG

#define DASSERT(cond, streamableCode, ...) \
	do { \
		if (0) ASSERT(cond, streamableCode, ## __VA_ARGS__); \
	} while(0)

#else

#define DASSERT(cond, streamableCode, ...) ASSERT(cond, streamableCode, ## __VA_ARGS__)

#endif // NDEBUG

#define FAILURE(msg, ...) ASSERT(false, msg, ## __VA_ARGS__)

#define ERRNO_ASSERT(cond, streamableCode, ...) \
	do { \
		int const savedErrno = errno; \
		ASSERT(cond, streamableCode, savedErrno, ## __VA_ARGS__); \
	} while(0)

#endif // #_SAFEKIDDO_UTILS_ASSERT_H

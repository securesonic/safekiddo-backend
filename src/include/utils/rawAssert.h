#ifndef _SAFEKIDDO_UTILS_RAW_ASSERT_H
#define _SAFEKIDDO_UTILS_RAW_ASSERT_H

#include <iostream>
#include <sstream>

// Really simple log macro to be used when log system may not be initialized.
#define RAW_LOG(streamableCode) \
	{\
		std::ostringstream oss; \
		oss << streamableCode << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
		std::cerr << oss.str(); \
	}

#define RAW_ASSERT(cond, streamableCode) \
	{\
		if (!(cond))\
		{\
			RAW_LOG("Assertion failed: <" << #cond << ">, with message: <" << streamableCode << ">"); \
			::abort(); \
		}\
	}

#ifdef NDEBUG
	#define RAW_DASSERT(cond, streamableCode) { if(0) RAW_ASSERT(cond, streamableCode) }
#else
	#define RAW_DASSERT(cond, streamableCode) RAW_ASSERT(cond, streamableCode)
#endif

#endif

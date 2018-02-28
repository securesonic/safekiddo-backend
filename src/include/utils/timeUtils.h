/*
 * timeUtils.h
 *
 *  Created on: 12 Feb 2014
 *      Author: zytka
 */

#ifndef _SAFEKIDDO_TIMEUTILS_H_
#define _SAFEKIDDO_TIMEUTILS_H_

#include <boost/date_time/posix_time/posix_time.hpp>


#define BEGIN_TIMED_BLOCK(prefix) \
	boost::posix_time::ptime const prefix ## StartTime = boost::posix_time::microsec_clock::local_time();

#define END_TIMED_BLOCK(prefix) \
	boost::posix_time::time_duration const prefix ## Duration = boost::posix_time::microsec_clock::local_time() - prefix ## StartTime;


#ifdef NDEBUG

#define DBG_BEGIN_TIMED_BLOCK(prefix)

#define DBG_END_TIMED_BLOCK(prefix) \
	boost::posix_time::time_duration const prefix ## Duration;

#else // NDEBUG

#define DBG_BEGIN_TIMED_BLOCK(prefix) BEGIN_TIMED_BLOCK(prefix)

#define DBG_END_TIMED_BLOCK(prefix) END_TIMED_BLOCK(prefix)

#endif // NDEBUG


#endif /* _SAFEKIDDO_TIMEUTILS_H_ */

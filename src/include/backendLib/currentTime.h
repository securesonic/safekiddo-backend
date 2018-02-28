/*
 * currentTime.h
 *
 *  Created on: Mar 21, 2014
 *      Author: witkowski
 */

#ifndef CURRENTTIME_H_
#define CURRENTTIME_H_

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/local_time/local_time.hpp>
namespace safekiddo
{
namespace backend
{

// current local time is set to <now>; for tests only
void setCurrentTime(boost::posix_time::ptime const & localTime);

// current UTC time
boost::posix_time::ptime getCurrentUtcTime();

// local time adjusted from UTC time
boost::posix_time::ptime convertToLocalTime(boost::posix_time::ptime const & utcTime);

// local time for given timezone adjusted from UTC time
boost::local_time::local_date_time convertToTimezoneTime(boost::posix_time::ptime const & utcTime, std::string const timezone);

} // namespace backend
} // namespace safekiddo


#endif /* CURRENTTIME_H_ */

/*
 * boostTime.cpp
 *
 *  Created on: 11 Apr 2014
 *      Author: sachanowicz
 */

#include "utils/boostTime.h"
#include "utils/assert.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace safekiddo
{
namespace utils
{

/*
 * For parsing methods, the argument must not be empty. A special value is returned in case of error.
 */
boost::posix_time::ptime parseDateTime(std::string const & dateTime)
{
	// boost parser does not handle invalid strings reliably
	// boost::posix_time::time_from_string(dateTime);
	DASSERT(!dateTime.empty(), "null value not expected");
	tm timetm;
	if (!strptime(dateTime.c_str(), "%Y-%m-%d %H:%M:%S", &timetm))
	{
		LOG(ERROR, "invalid dateTime: " << dateTime);
		return boost::posix_time::ptime(boost::posix_time::not_a_date_time);
	}
	return boost::posix_time::ptime_from_tm(timetm);
}

boost::posix_time::time_duration parseTime(std::string const & time)
{
	// boost::posix_time::duration_from_string(time);
	DASSERT(!time.empty(), "null value not expected");
	tm timetm;
	if (!strptime(time.c_str(), "%H:%M:%S", &timetm))
	{
		LOG(ERROR, "invalid time: " << time);
		return boost::posix_time::time_duration(boost::posix_time::not_a_date_time);
	}
	return boost::posix_time::time_duration(timetm.tm_hour, timetm.tm_min, timetm.tm_sec);
}

boost::gregorian::date parseDate(std::string const & date)
{
	// boost::gregorian::from_string(date);
	DASSERT(!date.empty(), "null value not expected");
	tm timetm;
	if (!strptime(date.c_str(), "%Y-%m-%d", &timetm))
	{
		LOG(ERROR, "invalid date: " << date);
		return boost::gregorian::date(boost::posix_time::not_a_date_time);
	}
	return boost::gregorian::date_from_tm(timetm);
}

} // namespace utils
} // namespace safekiddo

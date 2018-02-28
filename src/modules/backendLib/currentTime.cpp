/*
 * currentTime.cpp
 *
 *  Created on: Mar 21, 2014
 *      Author: witkowski
 */

#include "backendLib/currentTime.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/optional.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "utils/assert.h"

using namespace boost::posix_time;
using namespace boost::local_time;

namespace safekiddo
{
namespace backend
{

#ifdef NDEBUG

void setCurrentTime(ptime const & localTime)
{
	FAILURE("cannot set current time in non-debug config");
}

ptime getCurrentUtcTime()
{
	// Let's have high precision time.
	return microsec_clock::universal_time();
}

#else

static boost::mutex mutex;
static boost::optional<ptime> fixedTime;

void setCurrentTime(ptime const & localTime)
{
	boost::mutex::scoped_lock lock(mutex);

	DASSERT(!localTime.is_special(), "time is invalid");
	time_duration const currentUtcOffset = boost::date_time::c_local_adjustor<ptime>::utc_to_local(localTime) - localTime;
	fixedTime = localTime - currentUtcOffset;
	LOG(WARNING, "local time fixed to " << localTime << " UTC " << currentUtcOffset);
}

ptime getCurrentUtcTime()
{
	boost::mutex::scoped_lock lock(mutex);

	if (fixedTime)
	{
		return fixedTime.get();
	}

	// Let's have high precision time.
	return microsec_clock::universal_time();
}

#endif // NDEBUG

ptime convertToLocalTime(ptime const & utcTime)
{
	return boost::date_time::c_local_adjustor<ptime>::utc_to_local(utcTime);
}

local_date_time convertToTimezoneTime(ptime const & utcTime, std::string const timezone)
{
	tz_database tz_db;
	tz_db.load_from_file("/sk/etc//date_time_zonespec.csv");
	time_zone_ptr tz = tz_db.time_zone_from_region(timezone);
	local_date_time local_time(utcTime, tz);
	return (local_time);
}

} // namespace backend
} // namespace safekiddo

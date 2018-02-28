/*
 * boostTime.h
 *
 *  Created on: 11 Apr 2014
 *      Author: sachanowicz
 */

#ifndef _SAFEKIDDO_BOOST_TIME_H_
#define _SAFEKIDDO_BOOST_TIME_H_

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>

#include <string>

namespace safekiddo
{
namespace utils
{

boost::posix_time::ptime parseDateTime(std::string const & dateTime);
boost::posix_time::time_duration parseTime(std::string const & time);
boost::gregorian::date parseDate(std::string const & date);

} // namespace utils
} // namespace safekiddo

#endif /* _SAFEKIDDO_BOOST_TIME_H_ */

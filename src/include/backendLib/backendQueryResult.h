#ifndef _SAFEKIDDO_BACKEND_BACKEND_QUERY_RESULT_H
#define _SAFEKIDDO_BACKEND_BACKEND_QUERY_RESULT_H

#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>

#include <utility>
#include <string>

#include "dbClient.h"

#include "proto/safekiddo.pb.h"

namespace safekiddo
{
namespace backend
{

struct BackendQueryResult
{
	typedef std::pair<db::ProfileId, std::string> profile_type;
	typedef std::pair<db::CategoryGroupId, std::string> category_group_type;

	// From the following fields all possibly useful information can be obtained (like server local time or unixtimestamp), so they should
	// be retained even if we decide to change request db schema.
	boost::posix_time::ptime utcTime;
	boost::posix_time::ptime childTime;

	boost::optional<protocol::messages::Response::Result> responseCode; // required field; optional only for debugging reasons
	boost::optional<profile_type> profileInfo;
	boost::optional<category_group_type> categoryGroupInfo;
};

} // namespace backend
} // namespace safekiddo

#endif // _SAFEKIDDO_BACKEND_BACKEND_QUERY_RESULT_H

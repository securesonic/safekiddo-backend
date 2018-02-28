#ifndef _SAFEKIDDO_PROTO_UTILS_H
#define _SAFEKIDDO_PROTO_UTILS_H

#include <ostream>
#include <string>

#include "utils/assert.h"

#include <google/protobuf/message.h>

namespace google
{
namespace protobuf
{

inline std::ostream & operator<<(std::ostream & out, google::protobuf::Message const & message)
{
	std::string const & qualifiedTypeName = message.GetTypeName();
	DASSERT(qualifiedTypeName.find_last_of('.') != std::string::npos, "Protobuf GetTypeName implementation changed", qualifiedTypeName);

	return out
		<< qualifiedTypeName.substr(qualifiedTypeName.find_last_of('.') + 1)
		<< "[" << message.ShortDebugString() << "]";
}

} // namespace protobuf
} // namespace google

#endif // _SAFEKIDDO_PROTO_UTILS_H

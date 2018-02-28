#include "utils/stringFunctions.h"
#include <cstring>

namespace safekiddo
{
namespace utils
{

bool isPrefixOf(std::string const & str1, std::string const & str2)
{
	return str2.size() >= str1.size() && str2.compare(0, str1.size(), str1) == 0;
}

bool isSuffixOf(std::string const & str1, std::string const & str2)
{
	return str2.size() >= str1.size() && str2.compare(str2.size() - str1.size(), str1.size(), str1) == 0;
}


bool isPrefixOfNoCase(std::string const & prefix, std::string const & string)
{
	return string.size() >= prefix.size() && strncasecmp(prefix.c_str(), string.c_str(), prefix.size()) == 0;
}

} // namespace utils
} // namespace safekiddo

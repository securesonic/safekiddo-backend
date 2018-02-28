#ifndef _SAFEKIDDO_STRINGFUNCTIONS_H_
#define _SAFEKIDDO_STRINGFUNCTIONS_H_

#include <string>

namespace safekiddo
{
namespace utils
{

bool isPrefixOf(std::string const & str1, std::string const & str2);
bool isSuffixOf(std::string const & str1, std::string const & str2);
bool isPrefixOfNoCase(std::string const & prefix, std::string const & string);

} // namespace utils
} // namespace safekiddo

#endif /* _SAFEKIDDO_STRINGFUNCTIONS_H_ */

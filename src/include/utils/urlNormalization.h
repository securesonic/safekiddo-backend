/*
 * urlNormalization.h
 *
 *  Created on: 15 Apr 2014
 *      Author: sachanowicz
 */

#ifndef URL_NORMALIZATION_H_
#define URL_NORMALIZATION_H_

#include <string>
#include <vector>

namespace safekiddo
{
namespace utils
{

void splitUrl(
	std::string const & url,
	std::string & protocolSpecification,
	std::string & hostNameWithOptPort,
	std::string & pathAndQueryString,
	std::string & fragmentId
);
std::string normalizeUrl(std::string const & url);
std::string const shortenUrl(std::string const & url);
void percentDecode(std::string const & str, std::string & result);
void splitPathAndQueryString(
	std::string const & pathAndQueryString,
	std::string & path,
	std::vector<std::string> & params
);

std::string joinUrlParts(
	std::string const & protocolSpecification,
	std::string const & hostNameWithOptPort,
	std::string const & path,
	std::vector<std::string> const & params,
	std::string const & fragmentId
);

} // namespace utils
} // namespace safekiddo

#endif /* URL_NORMALIZATION_H_ */

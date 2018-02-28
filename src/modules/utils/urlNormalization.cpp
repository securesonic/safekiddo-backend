/*
 * urlNormalization.cpp
 *
 *  Created on: 15 Apr 2014
 *      Author: sachanowicz
 */

#include "utils/urlNormalization.h"
#include "utils/assert.h"
#include "utils/foreach.h"

#include <stddef.h>
#include <ctype.h>

#include <algorithm>

namespace safekiddo
{
namespace utils
{

/**
 * decode a percent-encoded C string with optional path normalization
 *
 * The buffer pointed to by @dst must be at least strlen(@src) bytes.
 * Decoding stops at the first character from @src that decodes to null.
 * Path normalization will remove redundant slashes and slash+dot sequences,
 * as well as removing path components when slash+dot+dot is found. It will
 * keep the root slash (if one was present) and will stop normalization
 * at the first questionmark found (so query parameters won't be normalized).
 *
 * @param dst       destination buffer
 * @param src       source buffer
 * @param normalize perform path normalization if nonzero
 * @return          number of valid characters in @dst
 * @author          Johan Lindh <johan@linkdata.se>
 * @legalese        BSD licensed (http://opensource.org/licenses/BSD-2-Clause)
 */
static ptrdiff_t urldecode(char* dst, const char* src, int normalize)
{
	char* org_dst = dst;
	int slash_dot_dot = 0;
	char ch, a, b;

	do
	{
		ch = *src++;
		if (ch == '%' && isxdigit(a = src[0]) && isxdigit(b = src[1]))
		{
			if (a < 'A') a -= '0';
			else if(a < 'a') a -= 'A' - 10;
			else a -= 'a' - 10;
			if (b < 'A') b -= '0';
			else if(b < 'a') b -= 'A' - 10;
			else b -= 'a' - 10;
			ch = 16 * a + b;
			src += 2;
		}

		if (normalize)
		{
			switch (ch)
			{
			case '/':
				if (slash_dot_dot < 3)
				{
					/* compress consecutive slashes and remove slash-dot */
					dst -= slash_dot_dot;
					slash_dot_dot = 1;
					break;
				}
				/* fall-through */
			case '?':
				/* at start of query, stop normalizing */
				if (ch == '?')
					normalize = 0;
				/* fall-through */
			case '\0':
				if (slash_dot_dot > 1)
				{
					/* remove trailing slash-dot-(dot) */
					dst -= slash_dot_dot;
					/* remove parent directory if it was two dots */
					if (slash_dot_dot == 3)
						while (dst > org_dst && *--dst != '/')
							/* empty body */;
					slash_dot_dot = (ch == '/') ? 1 : 0;
					/* keep the root slash if any */
					if (!slash_dot_dot && dst == org_dst && *dst == '/')
						++dst;
				}
				break;
			case '.':
				if (slash_dot_dot == 1 || slash_dot_dot == 2)
				{
					++slash_dot_dot;
					break;
				}
				/* fall-through */
			default:
				slash_dot_dot = 0;
			}
		}
		*dst++ = ch;
	} while(ch);

	return (dst - org_dst) - 1;
}

static bool isValidUriScheme(std::string const & scheme)
{
	FOREACH_CREF(chr, scheme)
	{
		if (!isalnum(chr) && chr != '.' && chr != '-')
		{
			return false;
		}
	}
	return true;
}

static size_t startPosWithoutProtocol(std::string const & url)
{
	size_t startPos;
	size_t slashPos = url.find('/');

	if (slashPos != std::string::npos && slashPos > 0 && url[slashPos - 1] == ':' &&
		slashPos + 1 < url.size() && url[slashPos + 1] == '/' &&
		isValidUriScheme(url.substr(0, slashPos - 1)))
	{
		startPos = slashPos + 2;
	}
	else
	{
		// no protocol specification
		startPos = 0;
	}

	return startPos;
}

static std::string fixMissingSlash(size_t startPos, std::string const & url)
{
	size_t slashPos = url.find('/', startPos);
	size_t endOfHostNamePos = url.find_first_of("?#", startPos);

	if(endOfHostNamePos < slashPos)
	{
		return url.substr(0, endOfHostNamePos) + "/" + url.substr(endOfHostNamePos);
	}

	return url;
}

/*
 * Splits url into parts: protocolSpecification, hostNameWithOptPort, pathAndQueryString, fragmentId.
 * pathAndQueryString is always not empty ("/" is root). Does not perform any normalization.
 * If url contains "#" then it is included as a first character of fragmentId.
 *
 * Example 1:
 * for url = "http://www.example.com:80/abcDEF%aaX?q=42#top"
 * returns:
 * protocolSpecification = "http://"
 * hostNameWithOptPort = "www.example.com:80"
 * pathAndQueryString = "/abcDEF%aaX?q=42"
 * fragmentId = "#top"
 *
 * Example 2:
 * for url = "www.example.com"
 * returns:
 * protocolSpecification = ""
 * hostNameWithOptPort = "www.example.com"
 * pathAndQueryString = "/"
 * fragmentId = ""
 */
void splitUrl(
	std::string const & url,
	std::string & protocolSpecification,
	std::string & hostNameWithOptPort,
	std::string & pathAndQueryString,
	std::string & fragmentId
)
{
	size_t const hostNamePos = startPosWithoutProtocol(url);

	protocolSpecification = url.substr(0, hostNamePos);

	std::string const fixedUrl = fixMissingSlash(hostNamePos, url);

	size_t const endOfHostNamePos = fixedUrl.find('/', hostNamePos);
	if (endOfHostNamePos == std::string::npos)
	{
		hostNameWithOptPort = fixedUrl.substr(hostNamePos);
		pathAndQueryString = "/";
		// url is fixed, so there must be no '?' and '#' in it
		fragmentId = "";
	}
	else
	{
		hostNameWithOptPort = fixedUrl.substr(hostNamePos, endOfHostNamePos - hostNamePos);
		size_t const fragmentIdPos = fixedUrl.find('#', endOfHostNamePos + 1);
		if (fragmentIdPos == std::string::npos)
		{
			pathAndQueryString = fixedUrl.substr(endOfHostNamePos);
			fragmentId = "";
		}
		else
		{
			pathAndQueryString = fixedUrl.substr(endOfHostNamePos, fragmentIdPos - endOfHostNamePos);
			fragmentId = fixedUrl.substr(fragmentIdPos);
		}
	}
}

/*
 * Performs the following normalizations:
 * - removes protocol specification
 * - removes port number
 * - removes fragment id (part of url after '#')
 * - performs normalization of pathAndQueryString component described in urldecode()
 * - as a last step makes all lowercase
 */
std::string normalizeUrl(std::string const & url)
{
	std::string protocolSpecification; // unused
	std::string hostNameWithOptPort;
	std::string pathAndQueryString;
	std::string fragmentId; // unused - not needed
	splitUrl(url, protocolSpecification, hostNameWithOptPort, pathAndQueryString, fragmentId);

	// put hostNameWithOptPort without port number into <normalized>
	std::string normalized = hostNameWithOptPort.substr(0, hostNameWithOptPort.find(':'));

	// url-decode pathAndQueryString and remove redundant /../ and /./ and //
	size_t const startPos = normalized.size();
	normalized.resize(startPos + pathAndQueryString.size() + 1); // +1 is needed for trailing '\0'
	size_t const decodedLen = urldecode(&normalized[startPos], pathAndQueryString.c_str(), 1);
	normalized.resize(startPos + decodedLen);
	DASSERT(normalized[startPos] == '/', "slash removed by urldecode?");

	// make lowercase
	std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);

	return normalized;
}

std::string const shortenUrl(std::string const & url)
{
	size_t const maxLen = 512;
	if(url.size () > maxLen)
	{
		return std::string(url, 0, maxLen);
	}
	return url;
}

void percentDecode(std::string const & str, std::string & result)
{
	result.resize(str.size() + 1);
	size_t const decodedLen = urldecode(&result[0], str.c_str(), 0);
	result.resize(decodedLen);
}

/*
 * Split pathAndQueryString into path and params. Params are simple strings. '=' is not interpreted.
 *
 * Example 1:
 * for pathAndQueryString = "/search?q=abc&s=0"
 * returns:
 * path = "/search"
 * params = {"q=abc", "s=0"}
 *
 * Example 2:
 * for pathAndQueryString = "/somepath/aaa"
 * returns:
 * path = "/somepath/aaa"
 * params = {}
 */
void splitPathAndQueryString(
	std::string const & pathAndQueryString,
	std::string & path,
	std::vector<std::string> & params
)
{
	params.clear();
	size_t lastDelimPos = pathAndQueryString.find('?');
	path = pathAndQueryString.substr(0, lastDelimPos);
	while (lastDelimPos != std::string::npos)
	{
		size_t const nextDelimPos = pathAndQueryString.find('&', lastDelimPos + 1);
		if (nextDelimPos == std::string::npos)
		{
			params.push_back(pathAndQueryString.substr(lastDelimPos + 1));
		}
		else
		{
			params.push_back(pathAndQueryString.substr(lastDelimPos + 1, nextDelimPos - lastDelimPos - 1));
		}
		lastDelimPos = nextDelimPos;
	}
}

std::string joinUrlParts(
	std::string const & protocolSpecification,
	std::string const & hostNameWithOptPort,
	std::string const & path,
	std::vector<std::string> const & params,
	std::string const & fragmentId
)
{
	std::string url = protocolSpecification + hostNameWithOptPort + path;

	char delim = '?';
	FOREACH_CREF(param, params)
	{
		url += delim;
		url += param;
		delim = '&';
	}

	url += fragmentId;

	return url;
}

} // namespace utils
} // namespace safekiddo

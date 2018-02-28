/*
 * urlInfo.h
 *
 *  Created on: 12 Feb 2014
 *      Author: zytka
 */

#ifndef _SAFEKIDDO_URLINFO_H_
#define _SAFEKIDDO_URLINFO_H_

#include <stdint.h>
#include "utils/typedInteger.h"
#include "categorization.h"

struct sBcUriInfoExData;

namespace safekiddo
{
namespace webroot
{

TYPED_INTEGER(Reputation, int32_t);

class UrlInfo
{
public:
	enum ClassificationSource
	{
		UNSET = 1000,
		LOCAL_IP,
		RTU_CACHE,
		LOCAL_DB,
		BCAP_CACHE,
		BCAP
	};

	UrlInfo(
		sBcUriInfoExData const & webrootUriInfoExData,
		uint32_t const numCategories,
		std::string const & matchedUrl
	);

	Categorizations const & getCategorizations() const;
	Reputation getReputation() const;
	ClassificationSource getClassificationSource() const;
	bool isMostSpecific() const;
	std::string const & getMatchedUrl() const;

	bool verify() const;

	friend std::ostream & operator<<(std::ostream & os, UrlInfo const & self);

private:
	Categorizations categorizations;
	Reputation reputation;
	ClassificationSource classificationSource;
	bool mostSpecific; // tells whether more specific urls (like subdomains) or longer paths have the same categorizations
	std::string matchedUrl;
};

std::ostream & operator<<(std::ostream & os, UrlInfo const & self);
std::ostream & operator<<(std::ostream & os, UrlInfo::ClassificationSource source);

} // namespace webroot
} // namespace safekiddo


#endif /* _SAFEKIDDO_URLINFO_H_ */

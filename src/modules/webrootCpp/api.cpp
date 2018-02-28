/*
 * api.cpp
 *
 *  Created on: 13 Feb 2014
 *      Author: zytka
 */


#include "webrootCpp/api.h"
#include "webrootCpp/urlInfo.h"
#include "bcsdklib.h"

#include "utils/assert.h"
#include "utils/containerPrinter.h"
#include "utils/debug.h"
#include "utils/alerts.h"

#include <stdint.h>
#include <cstring>

namespace safekiddo
{
namespace webroot
{

namespace su = safekiddo::utils;

UrlInfo BcLookup(std::string const & url, int & socket, uint32_t maxCloudThreads)
{
	t_BcUriInfoExType uriInfo;
	std::string matchedUrl; // url matched by webroot machinery

	// nParseCatConfArray assumes this struct is zeroed
	memset(&uriInfo, 0, sizeof(uriInfo));
	// webroot modifies the string in-place to be lowercase
	std::string urlForWebroot = url;
	uint32_t numCategories = ::BcLookup(
		// don't use c_str() here as it does not unshare the string
		&urlForWebroot[0],
		0xDEADBEEF, // ignored
		&uriInfo,
		socket,
		matchedUrl,
		maxCloudThreads
	);

	DLOG(1, "categorization for: " << url);
	BEGIN_DEBUG
		std::ostringstream os;
		for (int32_t i = 0; i < nMaxCatsPerUrl; i++)
		{
			os << "(" << std::hex << uriInfo.aCC[i].CC << ")";
		}
		DLOG(1, "raw data: " << os.str(), numCategories, uriInfo.bcri, uriInfo.a1cat);
	END_DEBUG

	DASSERT(
		numCategories == 0 ||
		(uriInfo.source >= ::LOCAL_IP && uriInfo.source <= ::UNCAT_CACHE),
		"classification source not set? ", url, uriInfo.source
	);

	if (numCategories == 1 && uriInfo.aCC[0].CC == 0)
	{
		// uncat case, the URL is cached but has no CC's
		numCategories = 0;
	}

	UrlInfo const urlInfo = UrlInfo(uriInfo, numCategories, matchedUrl);

	if (!urlInfo.verify())
	{
		ALERT("got invalid categorization from webroot: ", url, urlInfo);
		// FIXME zytka 2014-05-01; add failure to catch up webroot problems early, before ALERTS are fully functional
		FAILURE("invalid categorization from webroot: ", url, urlInfo);
	}

	if (numCategories > 0)
	{
		DLOG(1, "got categorization: " << su::makeContainerPrinter(urlInfo.getCategorizations()) <<
			" from " << urlInfo.getClassificationSource(), url);
	}
	else if (urlInfo.getClassificationSource() == UrlInfo::BCAP)
	{
		LOG(INFO, "cloud unable to categorize", url);
	}
	else
	{
		DLOG(1, "uncategorized url from " << urlInfo.getClassificationSource(), url);
	}

	return urlInfo;
}

} // namespace webroot
} // namespace safekiddo


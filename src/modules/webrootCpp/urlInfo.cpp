/*
 * urlInfo.cpp
 *
 *  Created on: 13 Feb 2014
 *      Author: zytka
 */

#include "webrootCpp/urlInfo.h"
#include "utils/containerPrinter.h"

#include "utils/foreach.h"

namespace safekiddo
{
namespace webroot
{

enum
{
	MAX_NUM_CATEGORIES_FOR_URL = nMaxCatsPerUrl // webroot constant; see bcsdklib.h
};

UrlInfo::UrlInfo(
	sBcUriInfoExData const & webrootUriInfoExData,
	uint32_t const numCategories,
	std::string const & matchedUrl
):
	categorizations(),
	reputation(0),
	classificationSource(ClassificationSource(webrootUriInfoExData.source)),
	mostSpecific(webrootUriInfoExData.a1cat != 0),
	matchedUrl(matchedUrl)
{
	DASSERT(
		numCategories == 0 ||
		(this->classificationSource >= UNSET && this->classificationSource <= BCAP),
		"unexpected: ", this->classificationSource
	);

	this->categorizations.reserve(numCategories);
	for (uint32_t i = 0; i < numCategories; i++)
	{
		TYPE_CC const cc = webrootUriInfoExData.aCC[i];
		this->categorizations.push_back(Categorization(cc));
	}
}

Categorizations const & UrlInfo::getCategorizations() const
{
	return this->categorizations;
}

Reputation UrlInfo::getReputation() const
{
	return this->reputation;
}

bool UrlInfo::isMostSpecific() const
{
	return this->mostSpecific;
}

std::string const & UrlInfo::getMatchedUrl() const
{
	return this->matchedUrl;
}

UrlInfo::ClassificationSource UrlInfo::getClassificationSource() const
{
	return this->classificationSource;
}

bool UrlInfo::verify() const
{
	FOREACH_CONST(categorization, this->categorizations)
	{
		if (!categorization->verify())
		{
			return false;
		}
	}
	return true;
}

std::ostream & operator<<(std::ostream & os, UrlInfo const & self)
{
	os << "cat: " << utils::makeContainerPrinter(self.getCategorizations()) << ", rep: "
		<< self.getReputation() << ", src=" << self.getClassificationSource();
	return os;
}

std::ostream & operator<<(std::ostream & os, UrlInfo::ClassificationSource source)
{
	switch (source)
	{
	case UNSET:
		return os << "UNSET";
	case LOCAL_IP:
		return os << "LOCAL_IP";
	case RTU_CACHE:
		return os << "RTU_CACHE";
	case LOCAL_DB:
		return os << "LOCAL_DB";
	case BCAP_CACHE:
		return os << "BCAP_CACHE";
	case BCAP:
		return os << "BCAP";
	}
	FAILURE("invalid classification source: " << static_cast<int>(source));
}

std::ostream & operator<<(std::ostream & os, Category category)
{
	switch (category)
	{
	case REAL_ESTATE:
		return os << "REAL_ESTATE";
	case COMPUTER_AND_INTERNET_SECURITY:
		return os << "COMPUTER_AND_INTERNET_SECURITY";
	case FINANCIAL_SERVICES:
		return os << "FINANCIAL_SERVICES";
	case BUSINESS_AND_ECONOMY:
		return os << "BUSINESS_AND_ECONOMY";
	case COMPUTER_AND_INTERNET_INFO:
		return os << "COMPUTER_AND_INTERNET_INFO";
	case AUCTIONS:
		return os << "AUCTIONS";
	case SHOPPING:
		return os << "SHOPPING";
	case CULT_AND_OCCULT:
		return os << "CULT_AND_OCCULT";
	case TRAVEL:
		return os << "TRAVEL";
	case ABUSED_DRUGS:
		return os << "ABUSED_DRUGS";
	case ADULT_AND_PORNOGRAPHY:
		return os << "ADULT_AND_PORNOGRAPHY";
	case HOME_AND_GARDEN:
		return os << "HOME_AND_GARDEN";
	case MILITARY:
		return os << "MILITARY";
	case SOCIAL_NETWORK:
		return os << "SOCIAL_NETWORK";
	case DEAD_SITES_DB_OPS_ONLY:
		return os << "DEAD_SITES_DB_OPS_ONLY";
	case INDIVIDUAL_STOCK_ADVICE_AND_TOOLS:
		return os << "INDIVIDUAL_STOCK_ADVICE_AND_TOOLS";
	case TRAINING_AND_TOOLS:
		return os << "TRAINING_AND_TOOLS";
	case DATING:
		return os << "DATING";
	case SEX_EDUCATION:
		return os << "SEX_EDUCATION";
	case RELIGION:
		return os << "RELIGION";
	case ENTERTAINMENT_AND_ARTS:
		return os << "ENTERTAINMENT_AND_ARTS";
	case PERSONAL_SITES_AND_BLOGS:
		return os << "PERSONAL_SITES_AND_BLOGS";
	case LEGAL:
		return os << "LEGAL";
	case LOCAL_INFORMATION:
		return os << "LOCAL_INFORMATION";
	case STREAMING_MEDIA:
		return os << "STREAMING_MEDIA";
	case JOB_SEARCH:
		return os << "JOB_SEARCH";
	case GAMBLING:
		return os << "GAMBLING";
	case TRANSLATION:
		return os << "TRANSLATION";
	case REFERENCE_AND_RESEARCH:
		return os << "REFERENCE_AND_RESEARCH";
	case SHAREWARE_AND_FREEWARE:
		return os << "SHAREWARE_AND_FREEWARE";
	case PEER_TO_PEER:
		return os << "PEER_TO_PEER";
	case MARIJUANA:
		return os << "MARIJUANA";
	case HACKING:
		return os << "HACKING";
	case GAMES:
		return os << "GAMES";
	case PHILOSOPHY_AND_POLITICAL_ADVOCACY:
		return os << "PHILOSOPHY_AND_POLITICAL_ADVOCACY";
	case WEAPONS:
		return os << "WEAPONS";
	case PAY_TO_SURF:
		return os << "PAY_TO_SURF";
	case HUNTING_AND_FISHING:
		return os << "HUNTING_AND_FISHING";
	case SOCIETY:
		return os << "SOCIETY";
	case EDUCATIONAL_INSTITUTIONS:
		return os << "EDUCATIONAL_INSTITUTIONS";
	case ONLINE_GREETING_CARDS:
		return os << "ONLINE_GREETING_CARDS";
	case SPORTS:
		return os << "SPORTS";
	case SWIMSUITS_AND_INTIMATE_APPAREL:
		return os << "SWIMSUITS_AND_INTIMATE_APPAREL";
	case QUESTIONABLE:
		return os << "QUESTIONABLE";
	case KIDS:
		return os << "KIDS";
	case HATE_AND_RACISM:
		return os << "HATE_AND_RACISM";
	case ONLINE_PERSONAL_STORAGE:
		return os << "ONLINE_PERSONAL_STORAGE";
	case VIOLENCE:
		return os << "VIOLENCE";
	case KEYLOGGERS_AND_MONITORING:
		return os << "KEYLOGGERS_AND_MONITORING";
	case SEARCH_ENGINES:
		return os << "SEARCH_ENGINES";
	case INTERNET_PORTALS:
		return os << "INTERNET_PORTALS";
	case WEB_ADVERTISEMENTS:
		return os << "WEB_ADVERTISEMENTS";
	case CHEATING:
		return os << "CHEATING";
	case GROSS:
		return os << "GROSS";
	case WEB_BASED_EMAIL:
		return os << "WEB_BASED_EMAIL";
	case MALWARE_SITES:
		return os << "MALWARE_SITES";
	case PHISHING_AND_OTHER_FRAUDS:
		return os << "PHISHING_AND_OTHER_FRAUDS";
	case PROXY_AVOIDANCE_AND_ANONYMIZERS:
		return os << "PROXY_AVOIDANCE_AND_ANONYMIZERS";
	case SPYWARE_AND_ADWARE:
		return os << "SPYWARE_AND_ADWARE";
	case MUSIC:
		return os << "MUSIC";
	case GOVERNMENT:
		return os << "GOVERNMENT";
	case NUDITY:
		return os << "NUDITY";
	case NEWS_AND_MEDIA:
		return os << "NEWS_AND_MEDIA";
	case ILLEGAL:
		return os << "ILLEGAL";
	case CONTENT_DELIVERY_NETWORKS:
		return os << "CONTENT_DELIVERY_NETWORKS";
	case INTERNET_COMMUNICATIONS:
		return os << "INTERNET_COMMUNICATIONS";
	case BOT_NETS:
		return os << "BOT_NETS";
	case ABORTION:
		return os << "ABORTION";
	case HEALTH_AND_MEDICINE:
		return os << "HEALTH_AND_MEDICINE";
	case CONFIRMED_SPAM_SOURCES:
		return os << "CONFIRMED_SPAM_SOURCES";
	case SPAM_URLS:
		return os << "SPAM_URLS";
	case UNCONFIRMED_SPAM_SOURCES:
		return os << "UNCONFIRMED_SPAM_SOURCES";
	case OPEN_HTTP_PROXIES:
		return os << "OPEN_HTTP_PROXIES";
	case DYNAMICALLY_GENERATED_CONTENT:
		return os << "DYNAMICALLY_GENERATED_CONTENT";
	case PARKED_DOMAINS:
		return os << "PARKED_DOMAINS";
	case ALCOHOL_AND_TOBACCO:
		return os << "ALCOHOL_AND_TOBACCO";
	case PRIVATE_IP_ADDRESSES:
		return os << "PRIVATE_IP_ADDRESSES";
	case IMAGE_AND_VIDEO_SEARCH:
		return os << "IMAGE_AND_VIDEO_SEARCH";
	case FASHION_AND_BEAUTY:
		return os << "FASHION_AND_BEAUTY";
	case RECREATION_AND_HOBBIES:
		return os << "RECREATION_AND_HOBBIES";
	case MOTOR_VEHICLES:
		return os << "MOTOR_VEHICLES";
	case WEB_HOSTING:
		return os << "WEB_HOSTING";
	}
	FAILURE("invalid category: " << static_cast<int>(category));
}

} // namespace webroot
} // namespace safekiddo

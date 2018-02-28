/*
 * dbclient.h
 *
 *  Created on: 17 Feb 2014
 *      Author: witkowski
 */

#ifndef DBCLIENT_H_
#define DBCLIENT_H_

#include <postgresql/libpq-fe.h>

#include <list>
#include <vector>
#include <map>
#include <string>
#include <ostream>
#include <stdint.h>
#include <ctime>

#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/local_time/local_time.hpp> 

#include "utils/result.h"
#include "utils/typedInteger.h"


namespace safekiddo
{
namespace backend
{
namespace db
{

TYPED_INTEGER(Age, uint8_t);
TYPED_INTEGER(ChildId, int32_t);
TYPED_INTEGER(ProfileId, int32_t);
TYPED_INTEGER(CategoryGroupId, int32_t);
typedef std::string UrlPatternId; // join on profile_url_list and url tables
//TYPED_INTEGER(UrlPatternId, int32_t);
TYPED_INTEGER(CategoryId, int32_t);
TYPED_INTEGER(Status, uint8_t);

typedef std::string Uuid;
typedef boost::optional<ProfileId> ProfileIdOption;


// TODO: Use std::string for error values

struct PreparedStatement
{
	char const * stmtName;
	char const * query;
	int nParams;
};

typedef std::vector<PreparedStatement> PreparedStatements;

extern PreparedStatements const workerPreparedStatements;

#define PG_FORMAT_TEXT		0
#define PG_FORMAT_BINARY	1

class DBConnection
{
public:
	typedef safekiddo::utils::Result<int, DBConnection> ResultType;

	static ResultType create(
		std::string const & host,
		uint16_t port,
		std::string const & dbName,
		std::string const & userName,
		std::string const & password,
		std::string const & sslMode
	);

	~DBConnection();

	typedef std::vector<std::string> row_t;
	typedef utils::Result<int, std::list<row_t> > QueryResult;

	void setSocketTimeout(boost::posix_time::time_duration const & timeout);

	QueryResult query(std::string const & q) const;

	QueryResult queryPrepared(
		char const * stmtName,
		int nParams,
		char const * const * paramValues,
		int const * paramLengths,
		int const * paramFormats
	) const;

	PGconn * getHandle() const;

	PGresult * execute(char const * query) const;

	PGresult * executePrepared(
		char const * stmtName,
		int nParams,
		char const * const * paramValues,
		int const * paramLengths,
		int const * paramFormats,
		int resultFormat
	) const;

	PGresult * getResult() const;

	void setPreparedStatements(PreparedStatements const & preparedStatements);

private:
	DBConnection(PGconn * conn);

	bool waitConnReadable() const;

	bool shouldRetryAfterFailure() const;

	QueryResult extractQueryResult(PGresult * result) const;

	void registerPreparedStatements() const;


	PGconn * conn;

	boost::posix_time::time_duration socketTimeout;

	PreparedStatements preparedStatements;
};

class Child
{
public:
	typedef safekiddo::utils::Result<int, Child> ResultType;

	// return empty value if child not found
	static ResultType create(DBConnection const & conn, Uuid const & uuid);

	ChildId getId() const;
	std::string const & getName() const;
	Age getAge() const;
	boost::gregorian::date const & getBirthDate() const;
	ProfileId getDefaultProfileId() const;
	Uuid const & getUuid() const;
	std::string const & getTimezone() const;
	Status getStatus() const;
    

private:
	Child(
		ChildId childId,
		std::string const & name,
		boost::gregorian::date const & birthDate,
		ProfileId defaultProfileId,
		Uuid const & uuid,
		std::string const & timezone,
        Status status
	);

	ChildId childId;
	std::string name;
	boost::gregorian::date birthDate;
	ProfileId defaultProfileId;
	Uuid uuid;
	std::string timezone;
    Status status;
};

std::ostream & operator<<(std::ostream & out, Child const & self);

ProfileIdOption getAgeProfileId(DBConnection const & conn, Child const & child);

bool hasInternetAccess(
	DBConnection const & conn,
	ChildId childId,
	boost::local_time::local_date_time const & childLocalTime
);

bool hasTemporaryInternetAccess(
        DBConnection const & conn,
        ChildId childId,
        boost::posix_time::ptime const & currentUtcTime
);

class Decision
{
public:
	// constructs an invalid decision
	Decision();

	static Decision blocked();
	static Decision allowed();

	bool isBlocked() const;

	bool isValid() const;

	bool operator==(Decision const & other) const;

	friend std::ostream & operator<<(std::ostream & os, Decision const decision);
private:
	enum Value
	{
		INVALID,
		BLOCKED,
		ALLOWED
	};

	Decision(Value value);

	Value value;
};

template<class Id>
class Decisions
{
public:
	typedef Id IdType;

	void setElementDecision(Id id, Decision decision);

	boost::optional<Decision> getElementDecisionIfExists(Id id) const;

	std::vector<Id> getIds() const;

	void applyOverrides(Decisions const & other);

	template<class Idd>
	friend std::ostream & operator<<(std::ostream & os, Decisions<Idd> const & decisions);
private:
	typedef std::map<Id, Decision> DecisionMap;

	DecisionMap decisionMap;
};

class Profile
{
public:
	typedef safekiddo::utils::Result<int, Profile> ResultType;

	static ResultType create(DBConnection const & conn, ProfileId profileId);

	ProfileId getId() const;
	std::string const & getName() const;
	Decisions<UrlPatternId> const & getDecisionsForUrls() const;
	Decisions<CategoryGroupId> const & getDecisionsForCategoryGroups() const;

	friend std::ostream & operator<<(std::ostream & os, Profile const & profile);

private:
	static safekiddo::utils::Result<int, Decisions<UrlPatternId> > createDecisionsForUrls(
		DBConnection const & conn,
		ProfileId profileId
	);

	static safekiddo::utils::Result<int, Decisions<CategoryGroupId> > createDecisionsForCategoryGroups(
		DBConnection const & conn,
		ProfileId profileId
	);

	Profile(
		ProfileId profileId,
		std::string const & name,
		Decisions<UrlPatternId> const & decisionsForUrls,
		Decisions<CategoryGroupId> const & decisionsForCategoryGroups
	);

	ProfileId profileId;
	std::string name;

	Decisions<UrlPatternId> decisionsForUrls;
	Decisions<CategoryGroupId> decisionsForCategoryGroups;
};

class UrlPattern
{
public:
	typedef safekiddo::utils::Result<int, std::vector<UrlPattern> > ResultType;

	static ResultType createFromIds(std::vector<UrlPatternId> const & urlPatternIds);

	/*
	 * General assumptions:
	 * - all strings are lowercase
	 * - url is urldecoded
	 * - multiple consecutive "/" are replaced with one "/"
	 * - path must begin with "/"
	 * - host name should not contain protocol specification (like http://) nor port number (like :8080)
	 *
	 * There are 2 types of patterns:
	 *
	 * MATCH_ALL_SUBDOMAINS matches urls of the form: (ANY ".")? hostName "/" ANY
	 * No path can be given (to be clear which matching pattern is more specific).
	 *
	 * MATCH_DOMAIN_WITH_PATH matches urls of the form: hostName path ANY
	 *
	 * Rules for deciding which matching pattern is more specific:
	 * - all MATCH_DOMAIN_WITH_PATH patterns are more specific than MATCH_ALL_SUBDOMAINS
	 * - for 2 matching MATCH_ALL_SUBDOMAINS, the one with longer hostName is more specific
	 * - for 2 matching MATCH_DOMAIN_WITH_PATH, the one with longer path is more specific
	 */
	enum Kind
	{
		// order is important here
		MATCH_ALL_SUBDOMAINS,
		MATCH_DOMAIN_WITH_PATH
	};

	static UrlPattern createAllSubdomains(
		UrlPatternId urlPatternId,
		std::string const & hostName
	);
	static UrlPattern createDomainWithPath(
		UrlPatternId urlPatternId,
		std::string const & hostName,
		std::string const & path
	);

	/*
	 * requestUrl:
	 * - must follow general assumptions
	 * - must contain at least "/" after host name
	 */
	bool matchesUrl(std::string const & requestUrl) const;

	/*
	 * Assumption: both UrlPattern-s match the same url.
	 */
	bool isMoreSpecificThan(UrlPattern const & other) const;

	UrlPatternId getUrlPatternId() const;

private:
	UrlPattern(
		UrlPatternId urlPatternId,
		std::string const & hostName,
		std::string const & path,
		Kind kind
	);

	bool hostNameMatches(std::string const & requestUrl, bool subDomainAllowed) const;
	bool pathMatches(std::string const & requestUrl) const;

	UrlPatternId urlPatternId;
	std::string hostName;
	std::string path;
	Kind kind;
};

class Category
{
public:
	typedef safekiddo::utils::Result<int, std::vector<Category> > ResultType;

	static ResultType list(DBConnection const & conn, std::vector<CategoryId> const & categoryIds);

	CategoryId getCategoryId() const;
	std::string const & getName() const;
	CategoryGroupId getCategoryGroupId() const;

private:
	Category(CategoryId categoryId, std::string const & name, CategoryGroupId categoryGroupId);

	CategoryId categoryId;
	std::string name;
	CategoryGroupId categoryGroupId;
};


class CategoryGroup
{
public:
	typedef safekiddo::utils::Result<int, CategoryGroup> ResultType;

	static ResultType Create(DBConnection const & conn, CategoryGroupId categoryGroupId);

	CategoryGroupId getId() const;
	std::string const & getName() const;

private:
	CategoryGroup(CategoryGroupId id, std::string const & name);

	CategoryGroupId groupId;
	std::string name;
};

} // namespace db
} // namespace utils
} // namespace safekiddo

#endif /* DBCLIENT_H_ */

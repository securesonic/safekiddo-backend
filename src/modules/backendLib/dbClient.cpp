/*
 * dbclient.cpp
 *
 *  Created on: 17 Feb 2014
 *      Author: witkowski
 */


#include "backendLib/dbClient.h"

#include "utils/assert.h"
#include "utils/foreach.h"
#include "utils/containerPrinter.h"
#include "utils/boostTime.h"
#include "utils/timeUtils.h"
#include "utils/socketUtils.h"
#include "utils/alerts.h"
#include "utils/stringFunctions.h"

#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include <stdexcept>
#include <sstream>

#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>


namespace su = safekiddo::utils;

namespace safekiddo
{
namespace backend
{
namespace db
{

/**************************** conversion ******************************/

static bool parseBool(std::string const & value)
{
	DASSERT(!value.empty(), "null value not expected");
	if (value == "t")
	{
		return true;
	}
	if (value == "f")
	{
		return false;
	}
	FAILURE("invalid boolean value: " << value);
}

/**************************** DBConnection ******************************/

DBConnection::DBConnection(PGconn * conn):
	conn(conn),
	socketTimeout(),
	preparedStatements()
{
}

DBConnection::ResultType DBConnection::create(
	std::string const & host,
	uint16_t port,
	std::string const & dbName,
	std::string const & userName,
	std::string const & password,
	std::string const & sslMode
)
{
	LOG(INFO, "connecting to db " << dbName << " on " << host << ":" << port);

	std::ostringstream oss;
	if (!host.empty())
	{
		oss << " host=" << host;
	}
	oss << " port=" << port;
	if (!dbName.empty())
	{
		oss << " dbname=" << dbName;
	}
	if (!userName.empty())
	{
		oss << " user=" << userName;
	}
	if (!password.empty())
	{
		oss << " password=" << password;
	}
	if (!sslMode.empty())
	{
		oss << " sslmode=" << sslMode;
	}
	std::string const connInfo = oss.str();
	DLOG(3, "db connection info: " << connInfo);
	PGconn * conn = PQconnectdb(connInfo.c_str());

	if (PQstatus(conn) == CONNECTION_BAD)
	{
		LOG(ERROR, "db connection problem: " << PQerrorMessage(conn));
		PQfinish(conn);
		return -1;
	}

	return ResultType::ValueType(new DBConnection(conn));
}

DBConnection::~DBConnection()
{
	PQfinish(this->conn);
}

void DBConnection::setSocketTimeout(boost::posix_time::time_duration const & timeout)
{
	DASSERT(timeout.ticks() > 0, "timeout must be positive");
	LOG(INFO, "setting socket timeout to " << timeout);
	this->socketTimeout = timeout;
}

PreparedStatements const workerPreparedStatements =
{
	{
		"get_child",
		"select child.id, name, birth_date, default_custom_code_profile_id, child.timezone, users.status from child join users on users.code_parent_id = child.code_parent_id where uuid=$1::varchar",
		1
	},
	{
		"check_tmp_access_control",
		"select start_date, duration from tmp_access_control where code_child_id=$1::integer and parent_accepted=true",
		1
	},
	{
		"check_access_control",
		// column values are irrelevant
		"select 1 from access_control where code_child_id=$1::integer and day_of_week=$2::integer and"
		"((start_time<=$3::time and end_time>=$3::time) or (start_time='00:00:00' and end_time='00:00:00'))",
		3
	},
	{
		"get_profile",
		"select name from profile where id=$1::integer",
		1
	},
	{
		"get_profile_url_list",
		"select url.address, profile_url_list.blocked from profile_url_list, url where"
		" profile_url_list.code_url_id = url.id and"
		" profile_url_list.code_profile_id=$1::integer",
		1
	},
	{
		"get_profile_categories_groups_list",
		"select code_categories_groups_id, blocked from profile_categories_groups_list where code_profile_id=$1::integer",
		1
	},
	{
		"get_category_group",
		"select name from categories_groups where id=$1::integer",
		1
	}
};

void DBConnection::setPreparedStatements(PreparedStatements const & preparedStatements)
{
	this->preparedStatements = preparedStatements;
	this->registerPreparedStatements();
}

void DBConnection::registerPreparedStatements() const
{
	for (auto preparedStatement : this->preparedStatements)
	{
		PGresult * result = PQprepare(
			this->conn,
			preparedStatement.stmtName,
			preparedStatement.query,
			preparedStatement.nParams,
			NULL // paramTypes
		);
		if (PQresultStatus(result) != PGRES_COMMAND_OK)
		{
			LOG(ERROR, "cannot register prepared statement " << preparedStatement.stmtName << ", query: " << preparedStatement.query <<
				", result: " << PQresStatus(PQresultStatus(result)));
			PQclear(result);
			//return;
			FAILURE("Failed to register prepared statement, probably db schema is outdated");
		}
		PQclear(result);
	}
}

PGresult * DBConnection::execute(char const * query) const
{
	PGresult * result = PQgetResult(this->conn);
	ASSERT(result == NULL, "unconsumed result: " << PQresStatus(PQresultStatus(result)));

	DBG_BEGIN_TIMED_BLOCK(dbQuery)
	if (!PQsendQuery(this->conn, query))
	{
		return NULL;
	}
	result = this->getResult();
	DBG_END_TIMED_BLOCK(dbQuery)
	DLOG(3, "query '" << query << "' returned " << PQntuples(result) << " rows", dbQueryDuration);
	return result;
}

PGresult * DBConnection::executePrepared(
	char const * stmtName,
	int nParams,
	char const * const * paramValues,
	int const * paramLengths,
	int const * paramFormats,
	int resultFormat
) const
{
	PGresult * result = PQgetResult(this->conn);
	ASSERT(result == NULL, "unconsumed result: " << PQresStatus(PQresultStatus(result)));

	DBG_BEGIN_TIMED_BLOCK(dbQuery)
	if (!PQsendQueryPrepared(
		this->conn,
		stmtName,
		nParams,
		paramValues,
		paramLengths,
		paramFormats,
		resultFormat
	))
	{
		return NULL;
	}
	result = this->getResult();
	DBG_END_TIMED_BLOCK(dbQuery)
	DLOG(3, "statement " << stmtName << " returned " << PQntuples(result) << " rows", dbQueryDuration);
	return result;
}

PGresult * DBConnection::getResult() const
{
	if (this->socketTimeout.ticks() > 0 && !this->waitConnReadable())
	{
		if (PQstatus(this->conn) == CONNECTION_BAD)
		{
			LOG(ERROR, "db connection failed: " << PQerrorMessage(this->conn));
		}
		else
		{
			LOG(ERROR, "db connection timeout: " << PQerrorMessage(this->conn));
		}
		return NULL;
	}

	PGresult * result = PQgetResult(this->conn);
	if (result)
	{
		DLOG(3, "got db result: " << PQresStatus(PQresultStatus(result)));
	}
	return result;
}

bool DBConnection::waitConnReadable() const
{
	DASSERT(this->socketTimeout.ticks() > 0, "zero reserved for no timeout");

	// the socket is internally non-blocking
	int const sockfd = PQsocket(this->conn);
	ASSERT(sockfd != -1, "db connection not open?");

	boost::posix_time::ptime const endTime = boost::posix_time::microsec_clock::local_time() + this->socketTimeout;

	while (PQisBusy(this->conn))
	{
		DLOG(3, "db connection is busy");

		struct pollfd input_fd;
		input_fd.fd = sockfd;
		input_fd.events = POLLIN;
		input_fd.revents = 0;

		boost::posix_time::time_duration const timeLeft = std::max(
			endTime - boost::posix_time::microsec_clock::local_time(),
			boost::posix_time::time_duration()
		);
		int const nReady = ::poll(&input_fd, 1, timeLeft.total_milliseconds());
		if (nReady == -1)
		{
			int const err = errno;
			ASSERT(err == EINTR, "unexpected poll error: " << err);
			continue;
		}
		if (nReady == 0)
		{
			return false;
		}
		DASSERT(nReady == 1, "poll returned " << nReady);
		if (!PQconsumeInput(this->conn))
		{
			return false;
		}
	}
	return true;
}

bool DBConnection::shouldRetryAfterFailure() const
{
	LOG(WARNING, "reconnecting to db");
	PQreset(this->conn);
	if (PQstatus(this->conn) == CONNECTION_BAD)
	{
		LOG(ERROR, "reconnecting failed: " << PQerrorMessage(this->conn));
		return false;
	}
	LOG(INFO, "reconnecting to db succeeded");
	this->registerPreparedStatements();
	return true;
}

DBConnection::QueryResult DBConnection::query(std::string const & q) const
{
	PGresult * result = NULL;
	while (true)
	{
		result = this->execute(q.c_str());
		if (PQresultStatus(result) == PGRES_TUPLES_OK)
		{
			break;
		}
		LOG(ERROR, "db query problem: " << PQresStatus(PQresultStatus(result)), q);
		PQclear(result);
		if (!this->shouldRetryAfterFailure())
		{
			return -1;
		}
	}
	return this->extractQueryResult(result);
}

DBConnection::QueryResult DBConnection::queryPrepared(
	char const * stmtName,
	int nParams,
	char const * const * paramValues,
	int const * paramLengths,
	int const * paramFormats
) const
{
	PGresult * result = NULL;
	while (true)
	{
		result = this->executePrepared(
			stmtName,
			nParams,
			paramValues,
			paramLengths,
			paramFormats,
			PG_FORMAT_TEXT // resultFormat
		);
		if (PQresultStatus(result) == PGRES_TUPLES_OK)
		{
			break;
		}
		LOG(ERROR, "db query problem: " << PQresStatus(PQresultStatus(result)), stmtName);
		PQclear(result);
		if (!this->shouldRetryAfterFailure())
		{
			return -1;
		}
	}
	return this->extractQueryResult(result);
}

DBConnection::QueryResult DBConnection::extractQueryResult(PGresult * result) const
{
	size_t const rows = PQntuples(result);
	size_t const columns = PQnfields(result);

	boost::shared_ptr<std::list<row_t> > ret(new std::list<row_t>);
	for (size_t r = 0; r < rows; ++r)
	{
		ret->push_back(row_t());
		row_t & row = ret->back();
		for (size_t c = 0; c < columns; ++c)
		{
			row.push_back(std::string(PQgetvalue(result, r, c)));
		}
	}

	PQclear(result);

	return ret;
}

PGconn * DBConnection::getHandle() const
{
	return this->conn;
}

/**************************** Child ******************************/

Child::Child(
	ChildId childId,
	std::string const & name,
	boost::gregorian::date const & birthDate,
	ProfileId defaultProfileId,
	Uuid const & uuid,
	std::string const & timezone,
    Status status
):
	childId(childId),
	name(name),
	birthDate(birthDate),
	defaultProfileId(defaultProfileId),
	uuid(uuid),
	timezone(timezone),
    status(status)
{
	DLOG(1, "created " << *this);
}

Child::ResultType Child::create(DBConnection const & conn, Uuid const & uuid)
{
	char const * paramValues[] = {
		uuid.c_str()
	};
	int paramFormats[] = {
		PG_FORMAT_TEXT
	};
	DBConnection::QueryResult queryResult = conn.queryPrepared(
		"get_child",
		1, // nParams
		paramValues,
		NULL, // paramLengths; no binary parameters
		paramFormats
	);

	if (queryResult.isError())
	{
		return queryResult.getError();
	}

	std::list<DBConnection::row_t> const & rows = *queryResult.getValue();

	if (rows.empty())
	{
		// child not found - empty result
		return ResultType::ValueType();
	}

	if (rows.size() != 1)
	{
		ALERT("found " << rows.size() << " child entries for uuid " << uuid);
		return -1;
	}
	DBConnection::row_t const & row = rows.front();

	ChildId const childId = boost::lexical_cast<ChildId>(row.at(0));

	boost::gregorian::date const birthDate = su::parseDate(row.at(2));
	if (birthDate.is_special())
	{
		LOG(ERROR, "invalid birth date: " << row.at(2), childId);
		return -1;
	}
    
    Status const status = boost::lexical_cast<Status>(row.at(5));
    
	ProfileId defaultProfileId = boost::lexical_cast<ProfileId>(row.at(3));

	return ResultType::ValueType(
		new Child(
			childId,
			row.at(1), // name
			birthDate,
			defaultProfileId,
			uuid,
			row.at(4), //timezone
            status //status
		)
	);
}

ChildId Child::getId() const
{
	return this->childId;
}

std::string const & Child::getName() const
{
	return this->name;
}

Age Child::getAge() const
{
	FAILURE("please fix to use getCurrentUtcTime()");
	boost::gregorian::date_duration duration = boost::posix_time::second_clock::local_time().date() - this->birthDate;
	int32_t years = duration.days() / 365;
	if (years < 0 || years > 200)
	{
		return -1;
	}
	return years;
}

boost::gregorian::date const & Child::getBirthDate() const
{
	return this->birthDate;
}

ProfileId Child::getDefaultProfileId() const
{
	return this->defaultProfileId;
}

Uuid const & Child::getUuid() const
{
	return this->uuid;
}

std::string const & Child::getTimezone() const
{
        return this->timezone;
}
    
Status Child::getStatus() const
{
        return this->status;
}


std::ostream & operator<<(std::ostream & out, Child const & self)
{
	return out << "Child["
		"id=" << self.getId() <<
		", name='" << self.getName() << "'"
		", birthDate=" << self.getBirthDate() <<
		", defaultProfileId=" << self.getDefaultProfileId() <<
		", uuid=" << self.getUuid() <<
		", timezone=" << self.getTimezone() <<
		"]";
}

/**************************** age profile ******************************/

ProfileIdOption getAgeProfileId(DBConnection const & conn, Child const & child)
{
	Age age = child.getAge();
	if (age == -1)
	{
		LOG(ERROR, "invalid birth date", child);
		return boost::none;
	}

	std::ostringstream sStream;
	sStream << "select code_profile_id from age_profile where from_age <= " << age << " and to_age >= " << age;

	DBConnection::QueryResult queryResult = conn.query(sStream.str());

	if (queryResult.isError())
	{
		LOG(ERROR, "cannot fetch age profile id", child);
		return boost::none;
	}

	std::list<DBConnection::row_t> const & rows = *queryResult.getValue();
	if (rows.empty())
	{
		LOG(WARNING, "age profile not defined", child);
		return boost::none;
	}
	DBConnection::row_t const & row = rows.front();

	return boost::lexical_cast<ProfileId>(row.at(0));
}

/**************************** internet access time limit ******************************/

bool hasTemporaryInternetAccess(
	DBConnection const & conn,
	ChildId childId,
	boost::posix_time::ptime const & currentUtcTime
)
{
	int32_t const childIdValue = htonl(childId.get());
	char const * paramValues[] = {
		reinterpret_cast<char const *>(&childIdValue)
	};
	int paramLengths[] = {
		sizeof(childIdValue)
	};
	int paramFormats[] = {
		PG_FORMAT_BINARY
	};
	DBConnection::QueryResult queryResult = conn.queryPrepared(
		"check_tmp_access_control",
		1, // nParams
		paramValues,
		paramLengths,
		paramFormats
	);

	if (queryResult.isError())
	{
		LOG(ERROR, "cannot check tmp_access_control", childId);
		return true;
	}

	FOREACH_CREF(row, *queryResult.getValue())
	{
		boost::posix_time::ptime const startDate = su::parseDateTime(row.at(0));
		boost::posix_time::time_duration const duration = su::parseTime(row.at(1));
		if (startDate.is_special() || duration.is_special())
		{
			LOG(ERROR, "invalid date or time", childId);
			continue;
		}
		if (currentUtcTime <= startDate + duration)
		{
			return true;
		}
	}
	return false;
}

bool hasInternetAccess(
	DBConnection const & conn,
	ChildId childId,
	boost::local_time::local_date_time const & childLocalTime
)
{
	// day of week 0..6
	short const dayOfWeek = childLocalTime.date().day_of_week().as_number();
	ASSERT(dayOfWeek >= 0 && dayOfWeek < 7, "invalid day of week " << dayOfWeek);

	int32_t const childIdValue = htonl(childId.get());
	int32_t const dayOfWeekValue = htonl(dayOfWeek);
	std::string const timeValue = boost::posix_time::to_simple_string(childLocalTime.local_time().time_of_day());
    
	char const * paramValues[] = {
		reinterpret_cast<char const *>(&childIdValue),
		reinterpret_cast<char const *>(&dayOfWeekValue),
		timeValue.c_str()
	};
	int paramLengths[] = {
		sizeof(childIdValue),
		sizeof(dayOfWeekValue),
		0 // ignored
	};
	int paramFormats[] = {
		PG_FORMAT_BINARY,
		PG_FORMAT_BINARY,
		PG_FORMAT_TEXT
	};
	DBConnection::QueryResult queryResult = conn.queryPrepared(
		"check_access_control",
		3, // nParams
		paramValues,
		paramLengths,
		paramFormats
	);

	if (queryResult.isError())
	{
		LOG(ERROR, "cannot check access_control", childId);
		return true;
	}

	std::list<DBConnection::row_t> const & rows = *queryResult.getValue();
	if (!rows.empty())
	{
		return true;
	}
	return false;
}

/**************************** Decision ******************************/

Decision::Decision():
	value(INVALID)
{
}

Decision Decision::blocked()
{
	return Decision(BLOCKED);
}

Decision Decision::allowed()
{
	return Decision(ALLOWED);
}

bool Decision::isBlocked() const
{
	DASSERT(this->isValid(), "invalid decision");
	return this->value == BLOCKED;
}

bool Decision::isValid() const
{
	return this->value != INVALID;
}

bool Decision::operator==(Decision const & other) const
{
	return this->isBlocked() == other.isBlocked();
}

Decision::Decision(Value value):
	value(value)
{
}

std::ostream & operator<<(std::ostream & os, Decision const decision)
{
	switch (decision.value)
	{
	case Decision::BLOCKED:
		return os << "BLOCKED";
	case Decision::ALLOWED:
		return os << "ALLOWED";
	case Decision::INVALID:
		return os << "INVALID";
	}
	FAILURE("got unexpected decision value: " << decision.value);
}


/**************************** Decisions ******************************/

template<class Id>
void Decisions<Id>::setElementDecision(Id id, Decision decision)
{
	this->decisionMap[id] = decision;
}

template<class Id>
boost::optional<Decision> Decisions<Id>::getElementDecisionIfExists(Id id) const
{
	typename DecisionMap::const_iterator it = this->decisionMap.find(id);
	if (it == this->decisionMap.end())
	{
		return boost::none;
	}
	return it->second;
}

template<class Id>
std::vector<Id> Decisions<Id>::getIds() const
{
	std::vector<Id> ret;
	FOREACH_CONST(it, this->decisionMap)
	{
		ret.push_back(it->first);
	}
	return ret;
}

template<class Id>
void Decisions<Id>::applyOverrides(Decisions const & other)
{
	FOREACH_CMAP(id, decision, other.decisionMap)
	{
		this->setElementDecision(id, decision);
	}
}

template<class Id>
std::ostream & operator<<(std::ostream & os, Decisions<Id> const & decisions)
{
	std::ostringstream oss;
	FOREACH_CMAP(id, decision, decisions.decisionMap)
	{
		oss << "(" << id << ": " << decision << ")";
	}
	os << oss.str();
	return os;
}

// explicit instantiations
template class Decisions<CategoryGroupId>;
template class Decisions<UrlPatternId>;
template std::ostream & operator<<(std::ostream &, Decisions<CategoryGroupId> const &);
template std::ostream & operator<<(std::ostream &, Decisions<UrlPatternId> const &);

/**************************** Profile ******************************/

Profile::Profile(
	ProfileId profileId,
	std::string const & name,
	Decisions<UrlPatternId> const & decisionsForUrls,
	Decisions<CategoryGroupId> const & decisionsForCategoryGroups
):
	profileId(profileId),
	name(name),
	decisionsForUrls(decisionsForUrls),
	decisionsForCategoryGroups(decisionsForCategoryGroups)
{
}

Profile::ResultType Profile::create(DBConnection const & conn, ProfileId profileId)
{
	int32_t const profileIdValue = htonl(profileId.get());
	char const * paramValues[] = {
		reinterpret_cast<char const *>(&profileIdValue)
	};
	int paramLengths[] = {
		sizeof(profileIdValue)
	};
	int paramFormats[] = {
		PG_FORMAT_BINARY
	};
	DBConnection::QueryResult queryResult = conn.queryPrepared(
		"get_profile",
		1, // nParams
		paramValues,
		paramLengths,
		paramFormats
	);

	if (queryResult.isError())
	{
		return queryResult.getError();
	}

	std::list<DBConnection::row_t> const & rows = *queryResult.getValue();
	if (rows.empty())
	{
		ALERT("profile not found", profileId);
		return -1;
	}
	DBConnection::row_t const & row = rows.front();

	su::Result<int, Decisions<UrlPatternId> > decisionsForUrlsResult = Profile::createDecisionsForUrls(conn, profileId);
	if (decisionsForUrlsResult.isError())
	{
		LOG(ERROR, "cannot access profile_url_list", profileId);
		return -1;
	}

	su::Result<int, Decisions<CategoryGroupId> > decisionsForCategoryGroupsResult =
		Profile::createDecisionsForCategoryGroups(conn, profileId);
	if (decisionsForCategoryGroupsResult.isError())
	{
		LOG(ERROR, "cannot access profile_categories_groups_list", profileId);
		return -1;
	}

	ResultType::ValueType profile = ResultType::ValueType(
		new Profile(
			profileId,
			row.at(0),
			*decisionsForUrlsResult.getValue(),
			*decisionsForCategoryGroupsResult.getValue()
		)
	);

	DLOG(1, "created profile: " << *profile);

	return profile;
}

su::Result<int, Decisions<UrlPatternId> > Profile::createDecisionsForUrls(
	DBConnection const & conn,
	ProfileId profileId
)
{
	int32_t const profileIdValue = htonl(profileId.get());
	char const * paramValues[] = {
		reinterpret_cast<char const *>(&profileIdValue)
	};
	int paramLengths[] = {
		sizeof(profileIdValue)
	};
	int paramFormats[] = {
		PG_FORMAT_BINARY
	};
	DBConnection::QueryResult queryResult = conn.queryPrepared(
		"get_profile_url_list",
		1, // nParams
		paramValues,
		paramLengths,
		paramFormats
	);

	if (queryResult.isError())
	{
		LOG(ERROR, "db query error");
		return -1;
	}

	boost::shared_ptr<Decisions<UrlPatternId> > decisions(new Decisions<UrlPatternId>);

	FOREACH_CREF(row, *queryResult.getValue())
	{
		Decision decision = parseBool(row.at(1)) ? Decision::blocked() : Decision::allowed();
		decisions->setElementDecision(row.at(0), decision);
	}

	return su::Result<int, Decisions<UrlPatternId> >(decisions);
}

su::Result<int, Decisions<CategoryGroupId> > Profile::createDecisionsForCategoryGroups(
	DBConnection const & conn,
	ProfileId profileId
)
{
	int32_t const profileIdValue = htonl(profileId.get());
	char const * paramValues[] = {
		reinterpret_cast<char const *>(&profileIdValue)
	};
	int paramLengths[] = {
		sizeof(profileIdValue)
	};
	int paramFormats[] = {
		PG_FORMAT_BINARY
	};
	DBConnection::QueryResult queryResult = conn.queryPrepared(
		"get_profile_categories_groups_list",
		1, // nParams
		paramValues,
		paramLengths,
		paramFormats
	);

	if (queryResult.isError())
	{
		LOG(ERROR, "db query error");
		return -1;
	}

	boost::shared_ptr<Decisions<CategoryGroupId> > decisions(new Decisions<CategoryGroupId>);

	FOREACH_CREF(row, *queryResult.getValue())
	{
		CategoryGroupId id = boost::lexical_cast<CategoryGroupId>(row.at(0));
		Decision decision = parseBool(row.at(1)) ? Decision::blocked() : Decision::allowed();
		decisions->setElementDecision(id, decision);
	}

	return su::Result<int, Decisions<CategoryGroupId> >(decisions);
}

ProfileId Profile::getId() const
{
	return this->profileId;
}

std::string const & Profile::getName() const
{
	return this->name;
}

Decisions<UrlPatternId> const & Profile::getDecisionsForUrls() const
{
	return this->decisionsForUrls;
}

Decisions<CategoryGroupId> const & Profile::getDecisionsForCategoryGroups() const
{
	return this->decisionsForCategoryGroups;
}

std::ostream & operator<<(std::ostream & os, Profile const & profile)
{
	os << "ProfileId=" << profile.profileId << ", name=" << profile.name << ", decisionsForUrl="
		<< profile.decisionsForUrls << ", decisionsForCategoryGroups=" << profile.decisionsForCategoryGroups;
	return os;
}

/**************************** UrlPattern ******************************/

UrlPattern::ResultType UrlPattern::createFromIds(std::vector<UrlPatternId> const & urlPatternIds)
{
	boost::shared_ptr<std::vector<UrlPattern> > urlPatterns(new std::vector<UrlPattern>);

	FOREACH_CREF(urlPatternId, urlPatternIds)
	{
		std::string const & address = urlPatternId;
		size_t const slashPos = address.find('/');
		// Note: it is assumed that url patterns in db are in normalized form except for the path component, which may be empty
		// (instead of '/') and it indicates MATCH_ALL_SUBDOMAINS kind of pattern.
		if (slashPos == std::string::npos)
		{
			urlPatterns->push_back(UrlPattern::createAllSubdomains(urlPatternId, address));
		}
		else
		{
			urlPatterns->push_back(UrlPattern::createDomainWithPath(
				urlPatternId,
				address.substr(0, slashPos),
				address.substr(slashPos, std::string::npos)
			));
		}
	}

	return urlPatterns;
}

UrlPattern UrlPattern::createAllSubdomains(
	UrlPatternId const urlPatternId,
	std::string const & hostName
)
{
	return UrlPattern(urlPatternId, hostName, std::string(), MATCH_ALL_SUBDOMAINS);
}

UrlPattern UrlPattern::createDomainWithPath(
	UrlPatternId const urlPatternId,
	std::string const & hostName,
	std::string const & path
)
{
	DASSERT(!path.empty() && path[0] == '/', "path must begin with '/'", path);
	return UrlPattern(urlPatternId, hostName, path, MATCH_DOMAIN_WITH_PATH);
}

bool UrlPattern::hostNameMatches(std::string const & requestUrl, bool subDomainAllowed) const
{
	size_t const slashPos = requestUrl.find('/');
	DASSERT(slashPos != std::string::npos, "requestUrl should contain '/'", requestUrl);
	return slashPos >= this->hostName.size() &&
		requestUrl.compare(slashPos - this->hostName.size(), this->hostName.size(), this->hostName) == 0 &&
		(slashPos == this->hostName.size() || (/* we know that slashPos > this->hostName.size() */
			subDomainAllowed && requestUrl[slashPos - this->hostName.size() - 1] == '.')
		);
}

bool UrlPattern::pathMatches(std::string const & requestUrl) const
{
	size_t const slashPos = requestUrl.find('/');
	DASSERT(slashPos != std::string::npos, "requestUrl should contain '/'", requestUrl);
	return requestUrl.size() - slashPos >= this->path.size() &&
		requestUrl.compare(slashPos, this->path.size(), this->path) == 0;
}

bool UrlPattern::matchesUrl(std::string const & requestUrl) const
{
	switch (this->kind)
	{
	case MATCH_ALL_SUBDOMAINS:
	{
		return this->hostNameMatches(requestUrl, true);
	}
	case MATCH_DOMAIN_WITH_PATH:
	{
		return this->hostNameMatches(requestUrl, false) && this->pathMatches(requestUrl);
	}
	default:
		FAILURE("not possible");
	}
}

bool UrlPattern::isMoreSpecificThan(UrlPattern const & other) const
{
	if (this->kind != other.kind)
	{
		return this->kind > other.kind;
	}
	switch (this->kind)
	{
	case MATCH_ALL_SUBDOMAINS:
		if (this->hostName.size() > other.hostName.size())
		{
			DASSERT(su::isSuffixOf(other.hostName, this->hostName), "only patterns matching the same url can be compared",
				other.hostName, this->hostName);
			return true;
		}
		else
		{
			DASSERT(su::isSuffixOf(this->hostName, other.hostName), "only patterns matching the same url can be compared",
				this->hostName, other.hostName);
			return false;
		}
	case MATCH_DOMAIN_WITH_PATH:
		DASSERT(this->hostName == other.hostName, "only patterns matching the same url can be compared",
			this->hostName, other.hostName);
		if (this->path.size() > other.path.size())
		{
			DASSERT(su::isPrefixOf(other.path, this->path), "only patterns matching the same url can be compared",
				other.path, this->path);
			return true;
		}
		else
		{
			DASSERT(su::isPrefixOf(this->path, other.path), "only patterns matching the same url can be compared",
				this->path, other.path);
			return false;
		}
	default:
		FAILURE("not possible");
	}
}

UrlPatternId UrlPattern::getUrlPatternId() const
{
	return this->urlPatternId;
}

UrlPattern::UrlPattern(
	UrlPatternId const urlPatternId,
	std::string const & hostName,
	std::string const & path,
	Kind const kind
):
	urlPatternId(urlPatternId),
	hostName(hostName),
	path(path),
	kind(kind)
{
}

/**************************** Category ******************************/

Category::ResultType Category::list(DBConnection const & conn, std::vector<CategoryId> const & categoryIds)
{
	boost::shared_ptr<std::vector<Category> > categories(new std::vector<Category>);
	if (categoryIds.empty())
	{
		return categories;
	}

	std::ostringstream sStream;
	sStream << "select ext_id, name, code_categories_groups_id from categories where ext_id in (" <<
		su::makeContainerPrinter(categoryIds) << ")";

	DBConnection::QueryResult queryResult = conn.query(sStream.str());
	if (queryResult.isError())
	{
		LOG(ERROR, "db query error");
		return -1;
	}

	FOREACH_CREF(row, *queryResult.getValue())
	{
		CategoryId categoryId = boost::lexical_cast<CategoryId>(row.at(0));
		CategoryGroupId categoryGroupId = boost::lexical_cast<CategoryGroupId>(row.at(2));
		categories->push_back(Category(categoryId, row.at(1), categoryGroupId));
	}

	return categories;
}

CategoryId Category::getCategoryId() const
{
	return this->categoryId;
}

std::string const & Category::getName() const
{
	return this->name;
}

CategoryGroupId Category::getCategoryGroupId() const
{
	return this->categoryGroupId;
}

Category::Category(CategoryId categoryId, std::string const & name, CategoryGroupId categoryGroupId):
	categoryId(categoryId),
	name(name),
	categoryGroupId(categoryGroupId)
{
}

CategoryGroup::CategoryGroup(CategoryGroupId id, const std::string &name)
	:	groupId(id),
		name(name)
{
}

CategoryGroupId CategoryGroup::getId() const
{
	return this->groupId;
}

std::string const & CategoryGroup::getName() const
{
	return this->name;
}

CategoryGroup::ResultType CategoryGroup::Create(DBConnection const & conn, CategoryGroupId groupId)
{
	int32_t const groupIdValue = htonl(groupId.get());
	char const * paramValues[] = {
		reinterpret_cast<char const *>(&groupIdValue)
	};
	int paramLengths[] = {
		sizeof(groupIdValue)
	};
	int paramFormats[] = {
		PG_FORMAT_BINARY
	};
	DBConnection::QueryResult queryResult = conn.queryPrepared(
		"get_category_group",
		1, // nParams
		paramValues,
		paramLengths,
		paramFormats
	);

	if (queryResult.isError())
	{
		LOG(ERROR, "db query error");
		return queryResult.getError();
	}

	std::list<DBConnection::row_t> const & rows = *queryResult.getValue();
	if (rows.size() != 1)
	{
		ALERT("found " << rows.size() << " categories_groups entries for id " << groupId);
		return -1;
	}
	DBConnection::row_t const & row = rows.front();

	return ResultType::ValueType(new CategoryGroup(groupId, row.at(0)));
}

} // namespace db
} // namespace backend
} // namespace safekiddo

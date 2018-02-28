#include <boost/thread/thread.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include "server.h"

#include "backendLib/currentTime.h"
#include "backendLib/dbClient.h"
#include "backendLib/webrootOverrides.h"

#include "utils/timeUtils.h"
#include "utils/assert.h"
#include "utils/foreach.h"
#include "utils/containerPrinter.h"
#include "utils/alerts.h"
#include "utils/urlNormalization.h"

#include "stats/statistics.h"

#include "proto/safekiddo.pb.h"
#include "proto/utils.h"

#include "webrootCpp/api.h"
#include "webrootCpp/config.h"

#include <queue>
#include <utility> //std::move

namespace su = safekiddo::utils;
namespace stats = safekiddo::statistics;

namespace safekiddo
{
namespace backend
{

static std::string const workersAddress = "inproc://workers";
static std::string const READY_STR = "READY";

static db::CategoryGroupId const uncategorizedGroupId = 27;

bool parseZmqRequest(zmq::message_t const & msg, protocol::messages::Request & request)
{
	bool const ret = request.ParseFromArray(msg.data(), msg.size());
	if (!ret)
	{
		LOG(ERROR, "cannot parse zmq request");
	}
	return ret;
}

zmq::message_t createZmqResponse(protocol::messages::Response const & response)
{
	zmq::message_t msg(response.ByteSize());
	response.SerializeWithCachedSizesToArray(static_cast<uint8_t*>(msg.data()));

	return msg;
}

template<class Id>
boost::optional<Id> anyElementContainsDecision(db::Decisions<Id> const & decisions, std::vector<Id> const & elements, db::Decision decision)
{
	FOREACH_CREF(element, elements)
	{
		boost::optional<db::Decision> elementDecision = decisions.getElementDecisionIfExists(element);
		if (elementDecision && elementDecision.get() == decision)
		{
			return element;
		}
	}
	return boost::none;
}


/*
 * In current implementation it returns at most one UrlPatternId, the most specific one. This solves the problem of multiple matching rules
 * for url in user request.
 */
std::vector<db::UrlPatternId> selectBestMatchingUrlPatternIds(
	std::vector<db::UrlPattern> const & profileUrlPatterns,
	std::string const & requestUrl
)
{
	boost::optional<db::UrlPattern> bestMatching;
	FOREACH_CREF(urlPattern, profileUrlPatterns)
	{
		if (urlPattern.matchesUrl(requestUrl))
		{
			if (!bestMatching || urlPattern.isMoreSpecificThan(*bestMatching))
			{
				bestMatching = urlPattern;
			}
		}
	}
	std::vector<db::UrlPatternId> ret;
	if (bestMatching)
	{
		ret.push_back(bestMatching->getUrlPatternId());
	}
	return ret;
}

su::Result<int, std::vector<db::CategoryGroupId> > getCategoryGroupIds(
	db::DBConnection const & conn,
	webroot::Categorizations const & categorizations
)
{
	std::vector<db::CategoryId> categoryIds;
	FOREACH_CREF(categorization, categorizations)
	{
		categoryIds.push_back(categorization.getCategory());
	}
	db::Category::ResultType categoriesListResult = db::Category::list(conn, categoryIds);
	if (categoriesListResult.isError())
	{
		return -1;
	}
	boost::shared_ptr<std::vector<db::CategoryGroupId> > categoryGroupIds(new std::vector<db::CategoryGroupId>);
	FOREACH_CREF(category, *categoriesListResult.getValue())
	{
		categoryGroupIds->push_back(category.getCategoryGroupId());
	}
	return su::Result<int, std::vector<db::CategoryGroupId> >(categoryGroupIds);
}

struct EffectiveDecisions
{
	db::Decisions<db::UrlPatternId> forUrls;
	db::Decisions<db::CategoryGroupId> forCategoryGroups;
};

su::Result<int, EffectiveDecisions> getEffectiveDecisions(
	db::DBConnection const & conn,
	db::Child const & child
)
{
	boost::shared_ptr<EffectiveDecisions> effectiveDecisions(new EffectiveDecisions);

	// depends on current time

	db::ProfileIdOption ageProfileId = db::getAgeProfileId(conn, child);

	db::ProfileIdOption profileIds[] = {
		ageProfileId,
		child.getDefaultProfileId()
		// TODO: calendar profile id for current time
	};

	for (uint32_t i = 0; i < sizeof(profileIds) / sizeof(profileIds[0]); ++i)
	{
		if (profileIds[i])
		{
			db::ProfileId profileId = *profileIds[i];
			db::Profile::ResultType profileResult = db::Profile::create(conn, profileId);
			if (profileResult.isError())
			{
				LOG(ERROR, "unable to get profile", profileId);
				return -1;
			}
			db::Profile const & profile = *profileResult.getValue();
			effectiveDecisions->forUrls.applyOverrides(profile.getDecisionsForUrls());
			effectiveDecisions->forCategoryGroups.applyOverrides(profile.getDecisionsForCategoryGroups());
		}
	}

	// TODO: tmp_*
	// TODO: global_*

	return su::Result<int, EffectiveDecisions>(effectiveDecisions);
}

using protocol::messages::Request;
using protocol::messages::Response;

/*
 * Decides if url should be let in or blocked. The algorithm is described on wiki page.
 */
BackendQueryResult queryDatabase(
	db::DBConnection const & conn,
	db::Child const & child,
	webroot::Categorizations const & categorizations,
	std::string const & requestUrl
)
{
	BackendQueryResult queryResult;

	boost::posix_time::ptime const utcTime = getCurrentUtcTime();
	queryResult.utcTime = utcTime;
	// Temporary untill we will have child local time
	queryResult.childTime = utcTime;


	LOG(INFO, "Query for child ",child.getId());
	
	// Child time in child local timezone
	boost::local_time::local_date_time const childLocalTime = convertToTimezoneTime(utcTime,child.getTimezone());
	// Child time as ptime
	boost::posix_time::ptime ptime_childLocalTime(childLocalTime.date(),childLocalTime.time_of_day());
	queryResult.childTime = ptime_childLocalTime;
 
	// Check if access is allowed at current time
	// Time passed as child local timezone time
	if (!db::hasInternetAccess(conn, child.getId(), childLocalTime))
	{
		// Check if temporary access is granted
		// Time as current UTC 
		if (!db::hasTemporaryInternetAccess(conn, child.getId(), utcTime))
		{
	                DLOG(1, "child " << child.getId() << " does not have internet access at the time: " << childLocalTime << ", UTC: "<< utcTime);
	                queryResult.responseCode = Response::RES_INTERNET_ACCESS_FORBIDDEN;
	                return queryResult;
		}
	}

	su::Result<int, db::Profile> defaultProfileResult = db::Profile::create(conn, child.getDefaultProfileId());
	if (defaultProfileResult.isError())
	{
		LOG(ERROR, "unable to get default profile", child.getUuid(), child.getDefaultProfileId());
		queryResult.responseCode = Response::RES_OK;
		return queryResult;
	}
	boost::shared_ptr<db::Profile> defaultProfile = defaultProfileResult.getValue();

	queryResult.profileInfo = BackendQueryResult::profile_type(
		defaultProfile->getId(),
		defaultProfile->getName()
	);

	EffectiveDecisions effectiveDecisions;
	// apply just one profile - defaultProfile
	effectiveDecisions.forUrls.applyOverrides(defaultProfile->getDecisionsForUrls());
	effectiveDecisions.forCategoryGroups.applyOverrides(defaultProfile->getDecisionsForCategoryGroups());
	// older code for previous design with age profile and more generally, for profiles overriding:
//	su::Result<int, EffectiveDecisions> effectiveDecisionsResult = getEffectiveDecisions(conn, child);
//	if (effectiveDecisionsResult.isError())
//	{
//		return protocol::messages::Response::RES_OK;
//	}
//	EffectiveDecisions const & effectiveDecisions = *effectiveDecisionsResult.getValue();

	su::Result<int, std::vector<db::CategoryGroupId> > categoryGroupIdsResult = getCategoryGroupIds(conn, categorizations);
	if (categoryGroupIdsResult.isError())
	{
		LOG(ERROR, "unable to get category group ids for categorizations: " << su::makeContainerPrinter(categorizations));
		queryResult.responseCode = Response::RES_OK;
		return queryResult;
	}

	// matchingCategoryGroupIds: which ids to check in effectiveDecisions.forCategoryGroups
	std::vector<db::CategoryGroupId> matchingCategoryGroupIds = *categoryGroupIdsResult.getValue();
	if (matchingCategoryGroupIds.empty())
	{
		// SAF-222
		matchingCategoryGroupIds.push_back(uncategorizedGroupId);
	}

	boost::optional<db::CategoryGroupId> blockedCategoryGroupId = anyElementContainsDecision(
		effectiveDecisions.forCategoryGroups,
		matchingCategoryGroupIds,
		db::Decision::blocked()
	);

	boost::optional<db::CategoryGroupId> selectedCategoryGroupId;
	if (blockedCategoryGroupId)
	{
		selectedCategoryGroupId = *blockedCategoryGroupId;
	}
	else if (!matchingCategoryGroupIds.empty())
	{
		// choose arbitrary one
		selectedCategoryGroupId = matchingCategoryGroupIds.front();
	}
	if (selectedCategoryGroupId)
	{
		db::CategoryGroup::ResultType categoryGroupResult = db::CategoryGroup::Create(conn, *selectedCategoryGroupId);
		if (categoryGroupResult.isError())
		{
			LOG(ERROR, "unable to get category group", *selectedCategoryGroupId);
			queryResult.responseCode = Response::RES_OK;
			return queryResult;
		}
		db::CategoryGroup const & categoryGroup = *categoryGroupResult.getValue();
		queryResult.categoryGroupInfo = BackendQueryResult::category_group_type(
			categoryGroup.getId(),
			categoryGroup.getName()
		);
	}

	db::UrlPattern::ResultType urlPatternListResult = db::UrlPattern::createFromIds(effectiveDecisions.forUrls.getIds());
	DASSERT(!urlPatternListResult.isError(), "in revised implementation it does not access db");

	// matchingUrlPatternIds: which ids to check in effectiveDecisions.forUrls
	std::vector<db::UrlPatternId> const matchingUrlPatternIds = selectBestMatchingUrlPatternIds(*urlPatternListResult.getValue(), requestUrl);

	if (anyElementContainsDecision(effectiveDecisions.forUrls, matchingUrlPatternIds, db::Decision::blocked()))
	{
		queryResult.responseCode = Response::RES_URL_BLOCKED_CUSTOM;
		return queryResult;
	}

	if (anyElementContainsDecision(effectiveDecisions.forUrls, matchingUrlPatternIds, db::Decision::allowed()))
	{
		queryResult.responseCode = Response::RES_OK;
		return queryResult;
	}

	if (blockedCategoryGroupId)
	{
		queryResult.responseCode = Response::RES_CATEGORY_BLOCKED_CUSTOM;
		return queryResult;
	}

	queryResult.responseCode = Response::RES_OK;
	return queryResult;
}

// SAF-222
static webroot::Categorizations filterOutPrivateIPs(webroot::Categorizations const & categorizations)
{
	webroot::Categorizations result;
	FOREACH_CREF(categorization, categorizations)
	{
		if (categorization.getCategory() != webroot::PRIVATE_IP_ADDRESSES)
		{
			result.push_back(categorization);
		}
	}
	return result;
}

static void handleEmptyMessage(zmq::socket_t & socket)
{
	zmq::message_t emptyMsg;
	socket.recv(&emptyMsg);
	ASSERT(emptyMsg.size() == 0, "handleEmptyMessage function received not empty message");
}

static std::string messageToString(zmq::message_t & msg)
{
	return std::string(static_cast<char*>(msg.data()), msg.size());
}

static zmq::message_t stringToMessage(std::string const & str)
{
	zmq::message_t toReturn(str.size());
	memcpy(toReturn.data(), str.data(), str.size());
	return toReturn;
}

void Server::workerFunction()
{
	LOG(INFO, "worker thread started");

	zmq::socket_t socket(this->context, ZMQ_REQ);
	socket.connect(workersAddress.c_str());

	// each worker maintains one connection for all requests
	db::DBConnection::ResultType connectionResult = db::DBConnection::create(
		this->config.dbHost,
		this->config.dbPort,
		this->config.dbName,
		this->config.dbUser,
		this->config.dbPassword,
		this->config.sslMode
	);

	if (connectionResult.isError())
	{
		LOG(ERROR, "unable to connect to db: " << connectionResult.getError());
		FAILURE("Cannot connect to database.");
	}

	LOG(INFO, "connected to profile db");

	db::DBConnection & connection = *connectionResult.getValue();
	connection.setSocketTimeout(boost::posix_time::seconds(5));
	connection.setPreparedStatements(db::workerPreparedStatements);

	int bcapSocketDescriptor = 0;

	try
	{
		zmq::message_t readyMsg = stringToMessage(READY_STR);
		socket.send(readyMsg);
	}
	catch (zmq::error_t const & error)
	{
		ASSERT(error.num() == ETERM, "unexpected zmq error: " << zmq_strerror(error.num()));
		return;
	}

	while (true)
	{
		DBG_BEGIN_TIMED_BLOCK(zmqRecv)
		//  Wait for next request from client
		// [clientAddress1][clientAddress2][empty][requestMsg]
		zmq::message_t clientAddress1;
		zmq::message_t clientAddress2;
		zmq::message_t requestMsg;
		try
		{
			socket.recv(&clientAddress1);
			socket.recv(&clientAddress2);
			handleEmptyMessage(socket);
			socket.recv(&requestMsg);
		}
		catch (zmq::error_t const & error)
		{
			ASSERT(error.num() == ETERM, "unexpected zmq error: " << zmq_strerror(error.num()));
			break;
		}
		DBG_END_TIMED_BLOCK(zmqRecv)

		protocol::messages::Request request;
		bool const parseOk = parseZmqRequest(requestMsg, request);
		if (!parseOk)
		{
			// FIXME: divide this method and add some reasonable handling of this error
			ALERT("cannot parse zmq request");
		}

		bool const logRequest = (request.useraction() == Request::INTENTIONAL_REQUEST);
		bool const categoryGroupQuery = (request.useraction() == Request::CATEGORY_GROUP_QUERY);

		std::string const notTooLongUrl = su::shortenUrl(request.url());
		std::string const normalizedUrl = su::normalizeUrl(notTooLongUrl);
        BackendQueryResult result;
        bool childQueryOK = true;
        
        DLOG(1, "received message from zmq", zmqRecvDuration, request);

        // Get child object
        
        db::Child::ResultType childResult = db::Child::create(connection, request.userid());
        if (childResult.isError())
        {
            LOG(ERROR, "unable to fetch child from db", request.userid());
            childQueryOK = false;
        }

        if (childQueryOK && !childResult.getValue())
        {
            DLOG(1, "child " << request.userid() << " not found in database");
            childQueryOK = false;
        }

        if (childQueryOK)
        {
            db::Child const & child = *childResult.getValue();

            webroot::Categorizations categorizations = this->webrootOverridesProvider.findCategorization(notTooLongUrl);
            
            // Query webroot if user status is not FREE
            if (child.getStatus() != SAFEKIDDO_USER_STATUS_FREE)
            {
                // query webroot if there are no overrides
                if (!categorizations.empty())
                {
                    this->hitBySourceStatisticsSender.addFromWebrootOverride();
                }
                else
                {
                    DBG_BEGIN_TIMED_BLOCK(bcLookup)
                    // webroot performs its own url decoding
                    webroot::UrlInfo const urlInfo = webroot::BcLookup(
                        notTooLongUrl,
                        bcapSocketDescriptor,
                        this->config.maxWorkersInCloud.get()
                    );
                    DBG_END_TIMED_BLOCK(bcLookup)
                    DLOG(1, "queried webroot", bcLookupDuration, notTooLongUrl, urlInfo.getClassificationSource());
                    this->hitBySourceStatisticsSender.addFromClassificationSource(urlInfo.getClassificationSource());
                    categorizations = urlInfo.getCategorizations();
                }

                categorizations = filterOutPrivateIPs(categorizations);
            }
            
            DBG_BEGIN_TIMED_BLOCK(queryDatabase)
            result = queryDatabase(
                connection,
                child,
                categorizations,
                normalizedUrl
            );
            DBG_END_TIMED_BLOCK(queryDatabase)
            DLOG(1, "queryDatabase returned " << *result.responseCode, queryDatabaseDuration, normalizedUrl);
            
        } else {
            result.responseCode = Response::RES_OK;
        }
        
		Response::Result const responseCode = *result.responseCode;


		protocol::messages::Response response;
		response.set_result(responseCode);

		if (responseCode == Response::RES_URL_BLOCKED_CUSTOM ||
			responseCode == Response::RES_CATEGORY_BLOCKED_CUSTOM)
		{
			DASSERT(result.profileInfo.is_initialized(), "profile info not set");
			response.set_usedprofileid(result.profileInfo->first.get());
			response.set_usedprofilename(result.profileInfo->second);
		}

		// [BUG id=SAF-351 fixer=sachanowicz date=2014-07-10 summary=don't crash on uncategorized site]
		if (result.categoryGroupInfo.is_initialized() &&
				(responseCode != Response::RES_OK || categoryGroupQuery))
		{
			response.set_categorygroupid(result.categoryGroupInfo->first.get());
			response.set_categorygroupname(result.categoryGroupInfo->second);
		}

		//  Sending response to client
		// [clientAddress1][clientAddress2][empty][response]
		try
		{
			zmq::message_t emptyMsg = zmq::message_t();
			zmq::message_t responseToSend = createZmqResponse(response);

			socket.send(clientAddress1, ZMQ_SNDMORE);
			socket.send(clientAddress2, ZMQ_SNDMORE);
			socket.send(emptyMsg, ZMQ_SNDMORE);
			socket.send(responseToSend);
		}
		catch (zmq::error_t const & error)
		{
			ASSERT(error.num() == ETERM, "unexpected zmq error: " << zmq_strerror(error.num()));
			break;
		}

		// asynchronous
		if (logRequest)
		{
			// pass original url here (not urldecoded) to keep only ascii characters (SAF-227)
			this->requestLogger.logRequest(notTooLongUrl, request.userid(), result);
		}
	}

	LOG(INFO, "worker thread finished");
}

Server::Server(
	Config const & config,
	statistics::StatisticsGatherer & statsGatherer,
	WebrootOverridesProvider & webrootOverridesProvider
):
	config(config),
	context(config.numIoThreads.get()),
	hitBySourceStatisticsSender(statsGatherer),
	requestLogger(config),
	statsGatherer(statsGatherer),
	webrootOverridesProvider(webrootOverridesProvider)
{
}

//More info: http://zguide.zeromq.org/cpp:lbbroker
void Server::loadBalancing(zmq::socket_t & clients, zmq::socket_t & workers)
{
	std::queue<zmq::message_t> worker_queue;

	while(true)
	{
		zmq::pollitem_t items [] = {
			// Always poll for worker activity on backend
			{ workers, 0, ZMQ_POLLIN, 0 },
			// Poll front-end only if we have available workers
			{ clients, 0, ZMQ_POLLIN, 0 }
		};

		try
		{
			worker_queue.empty() ? zmq::poll (&items[0], 1, -1) : zmq::poll (&items[0], 2, -1);
		}
		catch (zmq::error_t const & error)
		{
			ASSERT(error.num() == ETERM, "unexpected zmq error: " << zmq_strerror(error.num()));
			break;
		}

		if (items[0].revents & ZMQ_POLLIN)
		{
			// Getting message from worker
			// [workerAddress][empty][clientAddress1 OR "READY"]
			zmq::message_t workerAddress;
			zmq::message_t clientAddress1;
			try
			{
				workers.recv(&workerAddress);
				handleEmptyMessage(workers);
				workers.recv(&clientAddress1);
			}
			catch (zmq::error_t const & error)
			{
				ASSERT(error.num() == ETERM, "unexpected zmq error: " << zmq_strerror(error.num()));
				break;
			}
			worker_queue.push(std::move(workerAddress));

			// If client reply, send rest back to frontend
			if (messageToString(clientAddress1).compare(READY_STR) != 0)
			{
				try
				{
					// Getting another part of response from worker:
					// [clientAddress2][empty][response]
					// Client have two addresses, because there is one router in httpd code
					zmq::message_t clientAddress2;
					zmq::message_t response;
					zmq::message_t emptyMsg = zmq::message_t();

					workers.recv(&clientAddress2);
					handleEmptyMessage(workers);
					workers.recv(&response);

					// Sends reply to client
					// [clientAddress1][clientAddress2][empty][response]
					clients.send(clientAddress1, ZMQ_SNDMORE);
					clients.send(clientAddress2, ZMQ_SNDMORE);
					clients.send(emptyMsg, ZMQ_SNDMORE);
					clients.send(response);
				}
				catch (zmq::error_t const & error)
				{
					ASSERT(error.num() == ETERM, "unexpected zmq error: " << zmq_strerror(error.num()));
					break;
				}
			}
		}

		if (items [1].revents & ZMQ_POLLIN)
		{
			try
			{
				// Now get next client request, route to LRU worker
				// Client request is [clientAddress1][clientAddress2][empty][request]
				zmq::message_t clientAddress1;
				zmq::message_t clientAddress2;
				zmq::message_t requestMsg;
				zmq::message_t emptyMsg = zmq::message_t();

				clients.recv(&clientAddress1);
				clients.recv(&clientAddress2);
				handleEmptyMessage(clients);
				clients.recv(&requestMsg);

				zmq::message_t workerAddress = std::move(worker_queue.front());
				worker_queue.pop();

				// Send to worker [workerAddress][empty][clientAddress1][clientAddress2][empty][request]
				workers.send(workerAddress, ZMQ_SNDMORE);
				workers.send(emptyMsg, ZMQ_SNDMORE);
				workers.send(clientAddress1, ZMQ_SNDMORE);
				workers.send(clientAddress2, ZMQ_SNDMORE);
				workers.send(emptyMsg, ZMQ_SNDMORE);
				workers.send(requestMsg);
			}
			catch (zmq::error_t const & error)
			{
				ASSERT(error.num() == ETERM, "unexpected zmq error: " << zmq_strerror(error.num()));
				break;
			}
		}
	}
}

void Server::run()
{
	LOG(INFO, "starting server");
	webroot::WebrootConfig webrootConfig = webroot::BcReadConfig(this->config.webrootConfigFilePath);
	webrootConfig.setOem(this->config.webrootOem);
	webrootConfig.setDevice(this->config.webrootDevice);
	webrootConfig.setUid(this->config.webrootUid);
	if (this->config.useCloud)
	{
		webrootConfig.enableBcap();
		webrootConfig.enableDbDownload();
		webrootConfig.enableRtu();
	}
	if (!this->config.useLocalDb)
	{
		webrootConfig.disableLocalDb();
	}

	LOG(INFO, "initializing webroot with config:");
	webroot::BcLogConfig(webrootConfig);

	webroot::BcInitialize(webrootConfig);
	// [BUG id=SAF-480 fixer=sachanowicz date=2014-10-02 summary=WebrootOverridesProvider requires webroot to be initialized]
	this->webrootOverridesProvider.start();

	webroot::BcWaitForDbLoadComplete(webrootConfig);
	this->webrootOverridesProvider.waitUntilReady();

	// start listening
	zmq::socket_t clients(this->context, ZMQ_ROUTER);
	clients.bind(this->config.serverUrl.c_str());

	zmq::socket_t workers(this->context, ZMQ_ROUTER);
	workers.bind(workersAddress.c_str());

	this->requestLogger.start();

	webroot::BcStartStatisticsSending(this->statsGatherer);
	this->hitBySourceStatisticsSender.startStatisticsSending();

	//  Launch pool of worker threads
	boost::thread_group workerThreads;
	for (NumWorkers threadIdx = 0; threadIdx < this->config.numWorkers; threadIdx++)
	{
		workerThreads.create_thread(boost::bind(&Server::workerFunction, this));
	}

	this->loadBalancing(clients, workers); // this blocks

	LOG(INFO, "waiting for workers to finish");
	workerThreads.join_all();

	webroot::BcStopStatisticsSending();

	this->requestLogger.stop();

	webroot::BcShutdown();

	LOG(INFO, "server stopped");
}

void Server::requestStop()
{
	this->context.close();
}

void Server::updateWebrootOverridesProvider()
{
	this->webrootOverridesProvider.forceUpdateMap();
}

} // namespace backend
} // namespace safekiddo

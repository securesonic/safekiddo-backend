#ifndef _SAFEKIDDO_BACKEND_SERVER_H
#define _SAFEKIDDO_BACKEND_SERVER_H

#include "cppzmq/zmq.hpp"

#include "backendLib/config.h"
#include "backendLib/hitBySourceStatisticsSender.h"
#include "backendLib/requestLogger.h"

#define SAFEKIDDO_USER_STATUS_FREE	10

namespace safekiddo
{

namespace statistics
{
class StatisticsGatherer;
}

namespace backend
{

class WebrootOverridesProvider;

class Server
{
public:
	explicit Server(
		Config const & config,
		statistics::StatisticsGatherer & statsGatherer,
		WebrootOverridesProvider & webrootOverridesProvider
	);

	void loadBalancing(zmq::socket_t & clients, zmq::socket_t & workers);
	void run();

	void requestStop();
	void updateWebrootOverridesProvider();

private:
	void workerFunction();

	Config config;
	zmq::context_t context;
	HitBySourceStatisticsSender hitBySourceStatisticsSender;
	RequestLogger requestLogger;
	statistics::StatisticsGatherer & statsGatherer;
	WebrootOverridesProvider & webrootOverridesProvider;
};

} // namespace backend
} // namespace safekiddo

#endif // _SAFEKIDDO_BACKEND_SERVER_H

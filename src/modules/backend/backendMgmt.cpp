#include "backendMgmt.h"
#include "server.h"

#include "backendLib/currentTime.h"

#include "utils/boostTime.h"
#include "utils/assert.h"

#include "stats/statistics.h"


namespace su = safekiddo::utils;

namespace safekiddo
{
namespace backend
{

BackendMgmt::BackendMgmt(
	std::string const & mgmtFifoPath,
	Server & server,
	statistics::StatisticsGatherer & statsGatherer
):
	su::MgmtInterface(mgmtFifoPath),
	server(server),
	statsGatherer(statsGatherer)
{
}

bool BackendMgmt::handleCommand(std::string const & cmdName, std::istringstream & args)
{
	if (cmdName == "reloadOverrides")
	{
		this->cmdReloadOverrides(args);
	}
	else if (cmdName == "reopenStatsFile")
	{
		this->cmdReopenStatsFile(args);
	}
	else if (cmdName == "setCurrentTime")
	{
		this->cmdSetCurrentTime(args);
	}
	else if (cmdName == "stop")
	{
		this->cmdStop(args);
	}
	else
	{
		return false;
	}
	return true;
}

void BackendMgmt::cmdSetCurrentTime(std::istringstream & args)
{
	std::string currentTime;
	std::getline(args, currentTime); // argument contains space
	boost::posix_time::ptime const localTime = su::parseDateTime(currentTime);
	if (localTime.is_special())
	{
		LOG(ERROR, "Invalid setCurrentTime argument: " << currentTime);
		return;
	}
	setCurrentTime(localTime);
}

void BackendMgmt::cmdReloadOverrides(std::istringstream &)
{
	this->server.updateWebrootOverridesProvider();
}

void BackendMgmt::cmdReopenStatsFile(std::istringstream &)
{
	this->statsGatherer.reopen();
}

void BackendMgmt::cmdStop(std::istringstream &)
{
	this->requestStop();
	this->server.requestStop();
}

} // namespace backend
} // namespace safekiddo

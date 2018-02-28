#ifndef _SAFEKIDDO_BACKEND_BACKEND_MGMT_H
#define _SAFEKIDDO_BACKEND_BACKEND_MGMT_H

#include "utils/management.h"

namespace safekiddo
{

namespace statistics
{
class StatisticsGatherer;
}

namespace backend
{

class Server;

class BackendMgmt: public utils::MgmtInterface
{
public:
	explicit BackendMgmt(
		std::string const & mgmtFifoPath,
		Server & server,
		statistics::StatisticsGatherer & statsGatherer
	);

protected:
	virtual bool handleCommand(std::string const & cmdName, std::istringstream & args);

private:
	void cmdSetCurrentTime(std::istringstream & args);
	void cmdReloadOverrides(std::istringstream & args);
	void cmdReopenStatsFile(std::istringstream & args);
	void cmdStop(std::istringstream & args);

	Server & server;
	statistics::StatisticsGatherer & statsGatherer;
};

} // namespace backend
} // namespace safekiddo

#endif // _SAFEKIDDO_BACKEND_BACKEND_MGMT_H

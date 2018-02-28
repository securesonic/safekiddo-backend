#include "httpdMgmt.h"

#include "utils/assert.h"


namespace su = safekiddo::utils;

namespace safekiddo
{
namespace webservice
{
HttpdMgmt::HttpdMgmt(
	std::string const & mgmtFifoPath,
	std::shared_ptr<AdmissionControlContainer> const & admissionControlContainerPtr
):
	su::MgmtInterface(mgmtFifoPath),
	admissionControlContainerPtr(admissionControlContainerPtr)
{
}

bool HttpdMgmt::handleCommand(std::string const & cmdName, std::istringstream & args)
{
	if (cmdName == "reloadAdmissionControlCustomSettingsFile")
	{
		this->cmdReloadAdmissionControlCustomSettingsFile();
	}
	else
	{
		return false;
	}
	return true;
}

void HttpdMgmt::cmdReloadAdmissionControlCustomSettingsFile()
{
	this->admissionControlContainerPtr->reloadCustomSettingsFile();
}

} // namespace webservice
} // namespace safekiddo

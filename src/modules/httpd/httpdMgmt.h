#ifndef _SAFEKIDDO_WEBSERVICE_HTTPD_MGMT_H
#define _SAFEKIDDO_WEBSERVICE_HTTPD_MGMT_H

#include "admissionControlContainer.h"
#include "utils/management.h"
#include <memory>

namespace safekiddo
{
namespace webservice
{

class HttpdMgmt: public utils::MgmtInterface
{
public:
	explicit HttpdMgmt(
		std::string const & mgmtFifoPath,
		std::shared_ptr<AdmissionControlContainer> const & admissionControlContainerPtr
	);

protected:
	virtual bool handleCommand(std::string const & cmdName, std::istringstream & args);

private:
	std::shared_ptr<AdmissionControlContainer> admissionControlContainerPtr;
	void cmdReloadAdmissionControlCustomSettingsFile();
};

} // namespace webservice
} // namespace safekiddo

#endif // _SAFEKIDDO_WEBSERVICE_HTTPD_MGMT_H

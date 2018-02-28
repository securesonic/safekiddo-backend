#ifndef _SAFEKIDDO_UTILS_MANAGEMENT_H
#define _SAFEKIDDO_UTILS_MANAGEMENT_H

#include <string>
#include <sstream>
#include <fstream>

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

namespace safekiddo
{
namespace utils
{

class MgmtInterface
{
public:
	explicit MgmtInterface(
		std::string const & mgmtFifoPath
	);

	virtual ~MgmtInterface();

	void start();

protected:
	virtual bool handleCommand(std::string const & cmdName, std::istringstream & args);

	void requestStop();

private:
	void threadFunction();

	void ensureFifoExists();

	void processCommand(std::string const & line);

	void cmdFlushLogs(std::istringstream & args);
	void cmdSetLogLevel(std::istringstream & args);
	void cmdReopenLog(std::istringstream & args);


	std::string const mgmtFifoPath;

	boost::scoped_ptr<boost::thread> mgmtThread;

	std::ifstream fifoFile;

	bool stopRequested;
};

} // namespace utils
} // namespace safekiddo

#endif // _SAFEKIDDO_UTILS_MANAGEMENT_H

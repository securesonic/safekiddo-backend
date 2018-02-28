/*
 * serverStatus.h
 *
 *  Created on: Jul 21, 2014
 *      Author: witkowski
 */

#ifndef SERVERSTATUS_H_
#define SERVERSTATUS_H_

#include <ctime>
#include <mutex>

namespace safekiddo
{
namespace webservice
{

class ServerStatus
{
private:
	std::mutex mutable mutex;
	time_t const maxTimeBetweenEvents;
	size_t const maxProblemEvents;
	size_t counter;
	time_t lastProblem;

public:
	ServerStatus(
			time_t maxTimeBetweenEvents = 60,
			size_t maxProblemEvents = 5);
	void reportProblem();
	bool statusOk() const;
};

} // namespace webservice
} // namespace safekiddo

#endif /* SERVERSTATUS_H_ */

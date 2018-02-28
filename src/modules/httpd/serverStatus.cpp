/*
 * serverStatus.cpp
 *
 *  Created on: Jul 21, 2014
 *      Author: witkowski
 */

#include "serverStatus.h"

namespace safekiddo
{
namespace webservice
{

ServerStatus::ServerStatus(time_t maxTimeBetweenEvents, size_t maxProblemEvents)
:	maxTimeBetweenEvents(maxTimeBetweenEvents),
 	maxProblemEvents(maxProblemEvents),
 	counter(0),
	lastProblem(0)
{
}

void ServerStatus::reportProblem()
{
	std::unique_lock<typeof(this->mutex)> ul(this->mutex);
	time_t now = std::time(nullptr);
	if (this->lastProblem + maxTimeBetweenEvents < now)
	{
		counter = 1;
	}
	else
	{
		++counter;
	}
	this->lastProblem = std::time(nullptr);
}

bool ServerStatus::statusOk() const
{
	std::unique_lock<typeof(this->mutex)> ul(this->mutex);
	time_t now = std::time(nullptr);

	if (this->lastProblem + maxTimeBetweenEvents >= now &&
		counter > maxProblemEvents)
	{
		return false;
	}

	return true;
}

}
}

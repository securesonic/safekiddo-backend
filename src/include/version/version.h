/*
 * version.h
 *
 *  Created on: 30 Jun 2014
 *      Author: zytka
 */

#ifndef VERSION_H_
#define VERSION_H_

#include <string>

namespace safekiddo
{
namespace version
{

std::string getVersion();
std::string getBuildDate();
std::string getBuildHost();
std::string getBuildConfig();
std::string getDescription();

} // namespace version
} // namespace safekiddo

#endif /* VERSION_H_ */

/*
 * debug.h
 *
 *  Created on: 19 Feb 2014
 *      Author: zytka
 */

#ifndef _SAFEKIDDO_DEBUG_H_
#define _SAFEKIDDO_DEBUG_H_

#ifdef NDEBUG
// release code
#define BEGIN_DEBUG if (0) {
#define END_DEBUG }


#else
// debug code

#define BEGIN_DEBUG {
#define END_DEBUG }

#endif // NDEBUG

#endif /* _SAFEKIDDO_DEBUG_H_ */

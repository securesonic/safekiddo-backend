/*
 * backtrace.h
 *
 *  Created on: 13 Feb 2014
 *      Author: zytka
 */

#ifndef _SAFEKIDDO_BACKTRACE_H_
#define _SAFEKIDDO_BACKTRACE_H_

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <execinfo.h>
#include <cxxabi.h>

namespace safekiddo
{
namespace utils
{

inline std::string demangle(std::string const mangledSymbolOrType)
{
	/*
	   char* __cxa_demangle(const char* mangled_name, char* output_buffer, size_t* length, int* status)
	   - mangled_name, a NUL-terminated character string containing the name to be demangled.
	   - output_buffer, a region of memory, allocated with malloc, of *length bytes, into which the demangled name is
			stored. If output_buffer is not long enough, it is expanded using realloc. output_buffer may instead be
			NULL; in that case, the demangled name is placed in a region of memory allocated with malloc.
	   - length, if length is non-NULL, the length of the buffer containing the demangled name is placed in *length.
	   - status, ...is set to one of the following values:
			*  0: The demangling operation succeeded.
			* -1: A memory allocation failure occurred.
			* -2: mangled_name is not a valid name under the C++ ABI mangling rules.
			* -3: One of the arguments is invalid.

	   Returns a pointer to the start of the NUL-terminated demangled name, or NULL if the demangling fails.
	   The caller is responsible for deallocating this memory using free.
	*/

	int32_t status;
	char * tmp = __cxxabiv1::__cxa_demangle(mangledSymbolOrType.c_str(), NULL, NULL, &status);
	if(tmp==NULL)
	{
		return mangledSymbolOrType;
	}

	std::string const retVal(tmp);
	::free(tmp);

	return retVal;
}

inline std::string backtrace()
{
	int32_t const NUM_PTRS = 1000;
	void* ptrBuf[NUM_PTRS];

	int32_t numPtrs = ::backtrace(ptrBuf, NUM_PTRS);

	if (!numPtrs)
	{
		return "<backtrace not available>";
	}

	char** funcArray = ::backtrace_symbols(ptrBuf, numPtrs);

	if (!funcArray)
	{
		return "<backtrace not available>";
	}

	std::string result;

	for (int32_t i = 1; i < numPtrs; i++)
	{
		std::string func = funcArray[i];

		if (func[0] != '[')
		{
			result += demangle(func) + "\n";
		}
	}

	free(funcArray);

	return result;
}

} // utils
} // safekiddo

#endif /* _SAFEKIDDO_BACKTRACE_H_ */

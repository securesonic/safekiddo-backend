/*
 * returnValue.h
 *
 *  Created on: 19 Feb 2014
 *      Author: witkowski
 */

#ifndef RESULT_H_
#define RESULT_H_

#include <boost/shared_ptr.hpp>

#include "assert.h"

namespace safekiddo
{
namespace utils
{

template <class Error, class Value>
class Result
{
public:
	typedef Error ErrorType;
	typedef boost::shared_ptr<Value> ValueType;

	Result(ErrorType const error);
	Result(ValueType const value);

	ErrorType getError() const;
	ValueType getValue() const;
	bool isError() const;

private:
	bool const haveError;
	ErrorType const error;
	ValueType const value;
};

template <class Error, class Value>
Result<Error, Value>::Result(ErrorType const error_):
	haveError(true),
	error(error_)
{
}

template <class Error, class Value>
Result<Error, Value>::Result(ValueType const value_):
	haveError(false),
	error(),
	value(value_)
{
}

template <class Error, class Value>
typename Result<Error, Value>::ErrorType Result<Error, Value>::getError() const
{
	ASSERT(this->haveError, "no error found");
	return this->error;
}

template <class Error, class Value>
typename Result<Error,Value>::ValueType Result<Error, Value>::getValue() const
{
	ASSERT(!this->haveError, "no value found");
	return this->value;
}

template <class Error, class Value>
bool Result<Error, Value>::isError() const
{
	return this->haveError;
}

}
}

#endif /* RESULT_H_ */

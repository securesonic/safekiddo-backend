/*
 * macros.h
 *
 *  Created on: 13 Feb 2014
 *      Author: zytka
 */

#ifndef _SAFEKIDDO_MACROS_H_
#define _SAFEKIDDO_MACROS_H_

#ifndef CHAOS_PP_VARIADICS
#define CHAOS_PP_VARIADICS
#endif

// TODO limit chaos includes ?
#include <chaos/preprocessor.h>

#define PAIR_FIRST(x, y) x
#define PAIR_SECOND(x, y) y

#define MAKE_STR(...) MAKE_STR1(__VA_ARGS__)
#define MAKE_STR1(...) #__VA_ARGS__

/**
 * Macros iteration
 *
 * Example:
 *    #define INSTANTIATE_CLASS(_, type, ClassTemplate) template class ClassTemplate<type>;
 *    #define INSTANTIATE_CLASS_FOR_FLOATS(_, ClassTemplate, __) ITERATE_ITERABLE_MACRO(INSTANTIATE_CLASS, ClassTemplate, float, double, long double)
 *    ITERATE_ITERABLE_MACRO(INSTANTIATE_CLASS_FOR_FLOATS, _anything_, Template1, Template2);
 * which is equivalent to:
 *    template class Template1<float>; template class Template1<double>; template class Template1<long double>;
 *    template class Template2<float>; template class Template2<double>; template class Template2<long double>; ;
 */
#define ITERATE_ITERABLE_MACRO(MACRO, param, ...) \
	CHAOS_PP_UNLESS(CHAOS_PP_IS_EMPTY_NON_FUNCTION(__VA_ARGS__)) \
	( \
		CHAOS_PP_EXPAND(CHAOS_PP_EXPR(CHAOS_PP_TUPLE_FOR_EACH_I(ITERATE_ITERABLE_MACRO_ITER, (__VA_ARGS__), MACRO, param))) \
	)
#define ITERATE_ITERABLE_MACRO_ITER(_unused_, index, value, MACRO, param) MACRO(index, value, param)


/**
 * GENERIC_TO_STREAM is helper macro that is created to simplify definition of useful logging macros.
 *
 * This macro prints arbitrary number of variables.
 *
 * Besides this macro takes two additional parameters:
 * - varNamesPrefixInBrackets: it could be e.g (this->) or (objectName.) or just () if we don't want any prefix,
 * - printCommaBeforeFirstArg: 1 if comma should be printed before first variable, 0 if not.
 * - printNamesAndValues: 1 if both: values and names of variables must be printed, 0 if only values,
 */

// FIXME: add support for printing containers (generic printer mechanism instead of passing value)
#define GENERIC_TO_STREAM_HELPER_HELPER(index, value, paramsPair) \
	<< CHAOS_PP_WHEN(CHAOS_PP_OR(index)(PAIR_FIRST paramsPair))(", ") CHAOS_PP_WHEN(PAIR_SECOND paramsPair)(MAKE_STR(value) "=") "" \
	<< (value)

#define GENERIC_TO_STREAM_HELPER(index, value, additionalParamsPair) \
	GENERIC_TO_STREAM_HELPER_HELPER(index, CHAOS_PP_DECODE(PAIR_FIRST additionalParamsPair)value, PAIR_SECOND additionalParamsPair)

#define GENERIC_TO_STREAM(varNamesPrefixInBrackets, printCommaBeforeFirstArg, printNamesAndValues, ...) \
	"" ITERATE_ITERABLE_MACRO(GENERIC_TO_STREAM_HELPER, (varNamesPrefixInBrackets, (printCommaBeforeFirstArg, printNamesAndValues)), ## __VA_ARGS__)

/**
 * converts varargs to stream in format of name=value, name=value
 * int a = 1; int b = 2;
 * CONVERT_ARGS_TO_STREAM("some message ", a, b) => "some message " << "a=" << a << ", b=" << b
 */
#define CONVERT_ARGS_TO_STREAM(...) GENERIC_TO_STREAM((), 1, 1, ## __VA_ARGS__)

#endif /* _SAFEKIDDO_MACROS_H_ */

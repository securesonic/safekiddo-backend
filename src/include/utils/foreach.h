#ifndef _SAFEKIDDO_UTILS_FOREACH_H
#define _SAFEKIDDO_UTILS_FOREACH_H

#include <boost/type_traits/is_same.hpp>

namespace safekiddo
{
namespace utils
{

/**
 * Compile-time select. Taken from Loki.
 */
template <bool flag, typename T, typename U>
struct Select
{
	typedef T Result;
};

template <typename T, typename U>
struct Select<false, T, U>
{
	typedef U Result;
};

namespace __foreach__
{
	template<class T>
	T * getTPtr(T &);

	template<class T>
	T const * getTPtr(T const &);

	template<class T>
	T * getTNotConstPtr(T const &);

	class RandomAccessIteratorNotAllowed;
	template<class IteratorType>
	typename Select<
		boost::is_same<typename IteratorType::iterator_category, std::random_access_iterator_tag>::value,
		RandomAccessIteratorNotAllowed,
		IteratorType>::Result
	nonRandomAccessIterator(IteratorType);
}


#define FOREACH(it,c) \
	for(bool __loop##it = true; __loop##it; ) \
	for(typeof(*safekiddo::utils::__foreach__::getTPtr(c)) & __col##it = (c); __loop##it; __loop##it = false) \
	for(typeof(__col##it.begin()) it = __col##it.begin(), __itend##it = __col##it.end(); it != __itend##it; ++it)

#define FOREACH_CONST(it,c) \
	for(bool __loop##it = true; __loop##it; ) \
	for(typeof(c) const & __col##it = (c); __loop##it; __loop##it = false) \
	for(typeof(__col##it.begin()) it = __col##it.begin(), __itend##it = __col##it.end(); it != __itend##it; ++it)

// note: __loop##ref == false in the 3rd for means "there was a break command in the user code"
#define FOREACH_REF_IMPL(ref,c, referenceQualifier) \
	for(bool __loop##ref = true; __loop##ref; ) \
	for(typeof((*safekiddo::utils::__foreach__::getTNotConstPtr(c))) referenceQualifier __col##ref = (c); __loop##ref; __loop##ref = false) \
	for(typeof(__col##ref.begin()) __it##ref = __col##ref.begin(), __itend##ref = __col##ref.end(); __loop##ref && __it##ref != __itend##ref; ++__it##ref) \
	for(typeof(*__col##ref.begin()) referenceQualifier ref = *__it##ref; !(__loop##ref = !__loop##ref);)

#define FOREACH_CREF(ref,c) \
	FOREACH_REF_IMPL(ref, c, const &)

#define FOREACH_REF(ref,c) \
	FOREACH_REF_IMPL(ref, c, &)

// note: __loop##key == false in the 3rd for means "there was a break command in the user code"
#define FOREACH_MAP_IMPL(key,value,c, referenceQualifier) \
	for(bool __loop##key = true, __loopOnce##key = true; __loop##key; ) \
	for(typeof((*safekiddo::utils::__foreach__::getTNotConstPtr(c))) referenceQualifier __col##key = (c); __loop##key; __loop##key = false) \
	for(typeof(__col##key.begin()) __it##key = __col##key.begin(), __itend##key = __col##key.end(); __loop##key && __it##key != __itend##key; ++__it##key) \
	for(typeof(__col##key.begin()->first) & key = __it##key->first; !(__loopOnce##key = !__loopOnce##key);) \
	for(typeof(__col##key.begin()->second) referenceQualifier value = __it##key->second; !(__loop##key = !__loop##key);)

#define FOREACH_CMAP(key,value,c) \
	FOREACH_MAP_IMPL(key,value,c, const &)

#define FOREACH_MAP(key,value,c) \
	FOREACH_MAP_IMPL(key,value,c, &)

#define FOREACH_SAFE(it,c) \
	for(bool __loop##it = true; __loop##it; ) \
	for(typeof(*safekiddo::utils::__foreach__::getTPtr(c)) & __col##it = (c); __loop##it; __loop##it = false) \
	for(typeof(safekiddo::utils::__foreach__::nonRandomAccessIterator(__col##it.begin())) __nextit##it = __col##it.begin(), __itend##it = __col##it.end(), it = __nextit##it; \
		__nextit##it != __itend##it && ((it = __nextit##it++), true); )

} // namespace utils
} // namespace safekiddo

#endif  // _SAFEKIDDO_UTILS_FOREACH_H

/*
 * type safe integers
 */


#ifndef _SAFEKIDDO_UTILS_TYPED_INTEGER_H
#define _SAFEKIDDO_UTILS_TYPED_INTEGER_H

#include <limits>
#include <iostream>
#include <boost/lexical_cast.hpp>

#ifdef DEBUG
	bool const VERIFICATION = true;
#else
	bool const VERIFICATION = false;
#endif

namespace safekiddo
{
namespace utils
{
/**
 * typesafe integer types
 *
 * TInt underlying integer type.
 * TTag type discriminator
 */
template <class TInt, class TTag, bool verf = VERIFICATION>
class TypedInteger
{
public:
	/**
	 * default constructed TypedIntegers == 0
	 */
	TypedInteger();

	// make it implicit for convenience?
	TypedInteger(TInt const& value);

	/**
	 * conversion to underlying type.
	 */
	TInt const& get() const;

	bool operator<(TypedInteger const& other) const;
	bool operator<=(TypedInteger const& other) const;
	bool operator==(TypedInteger const& other) const;
	bool operator!=(TypedInteger const& other) const;
	bool operator>=(TypedInteger const& other) const;
	bool operator>(TypedInteger const& other) const;

	TypedInteger& operator++();
	TypedInteger operator++(int32_t);

	TypedInteger& operator--();
	TypedInteger operator--(int32_t);

	TypedInteger operator+(TypedInteger const& other) const;
	TypedInteger operator-(TypedInteger const& other) const;
	TypedInteger operator*(TypedInteger const& other) const;
	TypedInteger operator/(TypedInteger const& other) const;

	TypedInteger & operator+=(TypedInteger const& other);
	TypedInteger & operator-=(TypedInteger const& other);
	TypedInteger & operator*=(TypedInteger const& other);
	TypedInteger & operator/=(TypedInteger const& other);

	std::string toString() const;
	static TypedInteger FromString(std::string);

	template <typename Int2, class TTag2, bool verf2>
	friend std::ostream & operator<<(std::ostream & os, TypedInteger<Int2, TTag2, verf2> const & self);

	template <typename Int2, class TTag2, bool verf2>
	friend std::istream & operator>>(std::istream & is, TypedInteger<Int2, TTag2, verf2> & self);

	/// Returns the maximum integer value available for the underlying type.
	static TypedInteger Max();

	/// Returns the minimum integer value available for the underlying type.
	static TypedInteger Min();

private:
	TInt value;

};

#define TYPED_INTEGER(name, intType) \
	struct TypedIntegerTag##name; \
	typedef safekiddo::utils::TypedInteger<intType, TypedIntegerTag##name> name

} // namespace utils
} // namespace safekiddo

namespace std
{

//specialize numeric_limits for TypedInteger
template <class TInt, class TTag, bool verf>
struct numeric_limits<safekiddo::utils::TypedInteger<TInt, TTag, verf> >
{
	static const bool is_specialized = true;
	typedef safekiddo::utils::TypedInteger<TInt, TTag, verf> TTInt;

	static TTInt min() throw() { return numeric_limits<TInt>::min(); }
	static TTInt max() throw() { return numeric_limits<TInt>::max(); }

	static const int32_t digits = numeric_limits<TInt>::digits;
	static const int32_t digits10 = numeric_limits<TInt>::digits10;
	static const bool is_signed = numeric_limits<TInt>::is_signed;
	static const bool is_integer = numeric_limits<TInt>::is_integer;
	static const bool is_exact = numeric_limits<TInt>::is_exact;
	static const int32_t radix = numeric_limits<TInt>::radix;
	static TTInt epsilon() throw() { return numeric_limits<TInt>::epsilon(); }
	static TTInt round_error() throw() { return numeric_limits<TInt>::round_error(); }

	static const int32_t min_exponent = numeric_limits<TInt>::min_exponent;
	static const int32_t min_exponent10 = numeric_limits<TInt>::min_exponent10;
	static const int32_t max_exponent = numeric_limits<TInt>::max_exponent;
	static const int32_t max_exponent10 = numeric_limits<TInt>::max_exponent10;

	static const bool has_infinity = numeric_limits<TInt>::has_infinity;
	static const bool has_quiet_NaN = numeric_limits<TInt>::has_quiet_NaN;
	static const bool has_signaling_NaN = numeric_limits<TInt>::has_signaling_NaN;
	static const float_denorm_style has_denorm = numeric_limits<TInt>::has_denorm;
	static const bool has_denorm_loss = numeric_limits<TInt>::has_denorm_loss;

	static TTInt infinity() throw() { return numeric_limits<TInt>::infinity(); }
	static TTInt quiet_NaN() throw() { return numeric_limits<TInt>::quiet_NaN(); }
	static TTInt signaling_NaN() throw() { return numeric_limits<TInt>::signaling_NaN(); }
	static TTInt denorm_min() throw() { return numeric_limits<TInt>::denorm_min(); }

	static const bool is_iec559 = numeric_limits<TInt>::is_iec559;
	static const bool is_bounded = numeric_limits<TInt>::is_bounded;
	static const bool is_modulo = numeric_limits<TInt>::is_modulo;

	static const bool traps = numeric_limits<TInt>::traps;
	static const bool tinyness_before = numeric_limits<TInt>::tinyness_before;
	static const float_round_style round_style = numeric_limits<TInt>::round_style;
};
} /* namespace std */

#include "utils/typedInteger.cpp"


#endif // _SAFEKIDDO_UTILS_TYPED_INTEGER_H

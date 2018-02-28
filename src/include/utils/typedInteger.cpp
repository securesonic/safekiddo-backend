/*
 * Implementation of typedInteger.h
 */

#include "utils/assert.h"

namespace safekiddo
{
namespace utils
{

template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf>::TypedInteger():
	value(0)
{
}

template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf>::TypedInteger(TInt const& value):
	value(value)
{
}

template <typename TInt, class TDisc, bool verf>
inline TInt const& TypedInteger<TInt, TDisc, verf>::get() const
{
	return this->value;
}

// Comparison operators.
template <typename TInt, class TDisc, bool verf>
inline bool TypedInteger<TInt, TDisc, verf>::operator<(TypedInteger const& other) const
{
	return this->value < other.value;
}

template <typename TInt, class TDisc, bool verf>
inline bool TypedInteger<TInt, TDisc, verf>::operator<=(TypedInteger const& other) const
{
	return this->value <= other.value;
}

template <typename TInt, class TDisc, bool verf>
inline bool TypedInteger<TInt, TDisc, verf>::operator==(TypedInteger const& other) const
{
	return this->value == other.value;
}

template <typename TInt, class TDisc, bool verf>
inline bool TypedInteger<TInt, TDisc, verf>::operator!=(TypedInteger const& other) const
{
	return this->value != other.value;
}

template <typename TInt, class TDisc, bool verf>
inline bool TypedInteger<TInt, TDisc, verf>::operator>=(TypedInteger const& other) const
{
	return this->value >= other.value;
}

template <typename TInt, class TDisc, bool verf>
inline bool TypedInteger<TInt, TDisc, verf>::operator>(TypedInteger const& other) const
{
	return this->value > other.value;
}

template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf>& TypedInteger<TInt, TDisc, verf>::operator++()
{
	ASSERT(!verf || (*this != TypedInteger::Max()), "overflow");
	++this->value;
	return *this;
}

template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf> TypedInteger<TInt, TDisc, verf>::operator++(int32_t)
{
	ASSERT(!verf || (*this != TypedInteger::Max()), "overflow");
	TInt const temp = this->value;
	this->value++;
	return TypedInteger(temp);
}

template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf>& TypedInteger<TInt, TDisc, verf>::operator--()
{
	ASSERT(!verf || (*this != TypedInteger::Min()), " overflow");
	--this->value;
	return *this;
}

template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf> TypedInteger<TInt, TDisc, verf>::operator--(int32_t)
{
	ASSERT(!verf || (*this != TypedInteger::Min()), "overflow");
	TInt temp = this->value;
	this->value--;
	return TypedInteger(temp);
}

// Binary arithmetic.
template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf>
TypedInteger<TInt, TDisc, verf>::operator+(TypedInteger<TInt, TDisc, verf> const& other) const
{
	TInt const retVal = this->value + other.value;
	ASSERT(
		!verf, // || FIXME: add proper assert
		"verification not implemented"
	);
	return TypedInteger(retVal);
}

template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf>
TypedInteger<TInt, TDisc, verf>::operator-(TypedInteger<TInt, TDisc, verf> const& other) const
{
	TInt const retVal = this->value - other.value;
	ASSERT(
		!verf, // || FIXME: add proper assert
		"verification not implemented"
	);
	return TypedInteger(retVal);
}

template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf>
TypedInteger<TInt, TDisc, verf>::operator*(TypedInteger<TInt, TDisc, verf> const& other) const
{
	TInt const retVal = this->value * other.value;
	ASSERT(
		!verf, // || FIXME: add proper assert
		"verification not implemented"
	);
	return TypedInteger(retVal);
}

template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf>
TypedInteger<TInt, TDisc, verf>::operator/(TypedInteger<TInt, TDisc, verf> const& other) const
{
	TInt const retVal = this->value / other.value;
	ASSERT(
		!verf, // || FIXME: add proper assert
		"verification not implemented"
	);
	return TypedInteger(retVal);
}


template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf> &
TypedInteger<TInt, TDisc, verf>::operator+=(TypedInteger<TInt, TDisc, verf> const& other)
{
	TInt const newVal = this->value + other.value;
	ASSERT(
		!verf, // || FIXME: add proper assert
		"verification not implemented"
	);
	this->value = newVal;
	return *this;
}
	
template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf> &
TypedInteger<TInt, TDisc, verf>::operator-=(TypedInteger<TInt, TDisc, verf> const& other)
{
	TInt const newVal = this->value - other.value;

	ASSERT(
		!verf, // || FIXME: add proper assert
		"verification not implemented"
	);
	this->value = newVal;
	return *this;
}

template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf> &
TypedInteger<TInt, TDisc, verf>::operator*=(TypedInteger<TInt, TDisc, verf> const& other)
{
	TInt const newVal = this->value * other.value;
	ASSERT(
		!verf, // || FIXME: add proper assert
		"verification not implemented"
	);
	this->value = newVal;
	return *this;
}

template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf> &
TypedInteger<TInt, TDisc, verf>::operator/=(TypedInteger<TInt, TDisc, verf> const & other)
{
	this->value /= other.value;
	return *this;
}


template <typename TInt, class TDisc, bool verf>
inline std::ostream& operator<<(std::ostream & os, TypedInteger<TInt, TDisc, verf> const & self)
{
	os << self.toString();

	if (self == TypedInteger<TInt, TDisc, verf>::Max())
	{
		os << "==Max";
	}
	return os;
}

template <typename Int2, class TTag2, bool verf2>
inline std::istream& operator>>(std::istream & is, TypedInteger<Int2, TTag2, verf2> & self)
{
	std::string s;
	is >> s;
	self = TypedInteger<Int2, TTag2, verf2>::FromString(s);
	return is;
}


template <typename TInt, class TDisc, bool verf>
inline std::string TypedInteger<TInt, TDisc, verf>::toString() const
{
	if (sizeof(TInt) <= sizeof(char))
	{
		int32_t i = static_cast<int32_t>(this->value);
		return boost::lexical_cast<std::string>(i);
	}
	else
	{
		return boost::lexical_cast<std::string>(this->value);
	}
}

template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf>
TypedInteger<TInt, TDisc, verf>::FromString(std::string const str)
{
	TypedInteger<TInt, TDisc, verf> value;
	if (sizeof(TInt) <= sizeof(char))
	{
		int32_t const i = boost::lexical_cast<int32_t>(str);
		// FIXME:: ASSERT(i fits in TInt)
		value = i;
	}
	else
	{
		value = boost::lexical_cast<TInt>(str);
	}
	return value;
}


template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf> TypedInteger<TInt, TDisc, verf>::Max()
{
	return std::numeric_limits<TInt>::max();
}

template <typename TInt, class TDisc, bool verf>
inline TypedInteger<TInt, TDisc, verf> TypedInteger<TInt, TDisc, verf>::Min()
{
	return std::numeric_limits<TInt>::min();
}

} // namespace utils
} // namespace safekiddo

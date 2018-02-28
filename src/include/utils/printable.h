#ifndef _SAFEKIDDO_UTILS_PRINTABLE_H
#define _SAFEKIDDO_UTILS_PRINTABLE_H

namespace safekiddo
{
namespace utils
{

class Printable
{
public:
	virtual ~Printable() {}

	virtual std::ostream & print(std::ostream & out) const = 0;
};

inline std::ostream & operator<<(std::ostream & out, Printable const & self)
{
	self.print(out);
	return out;
}

template<typename StreamableT>
std::string getStringFromStreamable(StreamableT const & streamable)
{
	std::stringstream stream;
	stream << streamable;
	return stream.str();
}

} // utils
} // safekiddo

#endif

#ifndef _SAFEKIDDO_UTILS_CONTAINER_PRINTER_H
#define _SAFEKIDDO_UTILS_CONTAINER_PRINTER_H

#include "printable.h"

namespace safekiddo
{
namespace utils
{

template <class TIter>
class ContainerPrinter : public Printable
{
public:
	ContainerPrinter(TIter const & start, TIter const & end, std::string const & separator);

	virtual std::ostream & print(std::ostream & out) const;

private:
	TIter const start;
	TIter const end;
	std::string const separator;
};

template <class TIter>
inline ContainerPrinter<TIter> makeContainerPrinter(
	TIter const & start,
	TIter const & end,
	std::string const & separator = ", "
)
{
	return ContainerPrinter<TIter>(start, end, separator);
}

template <class TCol>
inline ContainerPrinter<typename TCol::const_iterator> makeContainerPrinter(
	TCol const & collection,
	std::string const & separator = ", "
)
{
	return makeContainerPrinter(collection.begin(), collection.end(), separator);
}

template <class TIter>
ContainerPrinter<TIter>::ContainerPrinter(
	TIter const & start,
	TIter const & end,
	std::string const & separator
):
	start(start),
	end(end),
	separator(separator)
{
}

template <class TIter>
std::ostream & ContainerPrinter<TIter>::print(std::ostream & out) const
{
	TIter iter = this->start;

	if (iter == this->end)
	{
		return out;
	}

	while (1)
	{
		out << *iter;
		++iter;

		if (iter != this->end)
		{
			out << this->separator;
		}
		else
		{
			break;
		}
	}

	return out;
}

} // utils
} // safekiddo

#endif

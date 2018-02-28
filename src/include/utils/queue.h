#ifndef _SAFEKIDDO_UTILS_QUEUE_H
#define _SAFEKIDDO_UTILS_QUEUE_H

#include <list>

namespace safekiddo
{
namespace utils
{

// list with O(1) size()
template<class T>
class Queue
{
public:
	Queue():
		impl(),
		numElements(0)
	{
	}

	T const & front() const
	{
		return this->impl.front();
	}

	T const & back() const
	{
		return this->impl.back();
	}

	void push_front(T const & elem)
	{
		this->impl.push_front(elem);
		++this->numElements;
	}

	void push_back(T const & elem)
	{
		this->impl.push_back(elem);
		++this->numElements;
	}

	void pop_front()
	{
		this->impl.pop_front();
		--this->numElements;
	}

	void pop_back()
	{
		this->impl.pop_back();
		--this->numElements;
	}

	bool empty() const
	{
		return this->numElements == 0;
	}

	size_t size() const
	{
		return this->numElements;
	}

	void clear()
	{
		this->impl.clear();
		this->numElements = 0;
	}

	void moveAllTo(std::list<T> & l, typename std::list<T>::iterator position)
	{
		l.splice(position, this->impl);
		this->numElements = 0;
	}

private:
	std::list<T> impl;
	size_t numElements;
};

} // namespace utils
} // namespace safekiddo

#endif // _SAFEKIDDO_UTILS_QUEUE_H

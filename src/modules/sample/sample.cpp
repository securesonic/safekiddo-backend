#include <iostream>
#include <boost/shared_ptr.hpp>

using namespace std;

int main()
{
	shared_ptr<int> p(new int(3));
	cout << *p << endl;
	return 0;
}


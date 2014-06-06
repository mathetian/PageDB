#include "../utils/Noncopyable.h"
#include "../utils/TestUtils.h"
using namespace utils;

#include "../utils/TestUtils.cpp"

class A : Noncopyable
{
};

TEST(A, Test1)
{
	A a;
	//A b = a;
	//A c(a);
	A d;
	//d = a;
}

int main()
{
	RunAllTests();
	return 0;
}
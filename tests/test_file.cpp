#include "FileModule.h"
#include "TestUtils.h"
using namespace utils;

#include "TestUtils.cpp"

class A { };

TEST(A, Test1)
{
    ASSERT_EQ(FileModule::Exist("Makefile"), true);
    ASSERT_EQ(FileModule::Exist("Makefile1"), false);

    ASSERT_NE(FileModule::Size("Makefile"), 0);
    ASSERT_EQ(FileModule::Size("Makefile2"), 0);
}

TEST(A, Test2)
{
	RandomFile file;
	file.Open("Hello.txt");
	string str = "123213\n";
	file.Write("123213\n", 0, str.size());
}

int main()
{
	RunAllTests();
	return 0;
}
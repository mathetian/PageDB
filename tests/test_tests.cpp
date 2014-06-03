#include "../utils/TestUtils.h"
using namespace utils;

#include "../utils/TestUtils.cpp"

class A
{
public:
    int func1()
    {
        return 1;
    }
    int func2()
    {
        return 2;
    }
};

TEST(A, abc1)
{
    ASSERT_TRUE(func1() == 1);
}

TEST(A, abc2)
{
    ASSERT_TRUE(func2() == 2);
}

int main()
{
    RunAllTests();
    return 0;
}
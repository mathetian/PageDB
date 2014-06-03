#include <vector>
using namespace std;

#include "Slice.h"
#include "BufferPacket.h"
using namespace customdb;

#include "TestUtils.h"
using namespace utils;

class A
{
public:
    int GetInt()
    {
        BufferPacket packet(sizeof(int));
        packet << 15545;
        Slice slice(packet.getData(), packet.getSize());
        return slice.returnAsInt();
    }

    int GetSize1()
    {
        Slice slice;
        return slice.size();
    }

    int GetSize2()
    {
        vector<Slice> slices = vector<Slice>(10,0);
        return slices.size();
    }


};

TEST(A, Test1)
{
    ASSERT_EQ(GetInt(), 15545);
}

TEST(A, Test2)
{
    ASSERT_EQ(GetSize1(), 0);
    ASSERT_EQ(GetSize2(), 10);
}

int main()
{
    RunAllTests();
    return 0;
}
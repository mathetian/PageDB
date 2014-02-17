#include <vector>
using namespace std;

#include "Slice.h"
#include "BufferPacket.h"

#include <assert.h>

#define EXPECT_EQ(a,b) assert(a == b)

void Test()
{
    BufferPacket packet(sizeof(int));
    packet << 15545;
    Slice slice(packet.getData(), packet.getSize());

    EXPECT_EQ(slice.returnAsInt(), 15545);

    Slice slice2("");
    EXPECT_EQ(slice2.size(),0);
}

void Test2()
{
    Slice slice;
    EXPECT_EQ(slice.size(), 0);
}

void Test3()
{
    vector<Slice> slices = vector<Slice>(10,0);
    EXPECT_EQ(slices.size(), 10);
    EXPECT_EQ(slices.at(0).size(), 0);
}

int main()
{
    Test2();
    printf("All Test has Passed\n");
    return 0;
}
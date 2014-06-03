#include <iostream>
using namespace std;

#include "Batch.h"
#include "Slice.h"
#include "BufferPacket.h"
using namespace customdb;

#include "TestUtils.h"
using namespace utils;

class A
{
public:
    WriteBatch* Test1()
    {
        BufferPacket packet(sizeof(int)*3);
        packet << 1;
        packet << 2;
        packet << 3;
        Slice k1(packet.getData(),sizeof(int));
        Slice k2(packet.getData() + sizeof(int), sizeof(int));
        Slice k3(packet.getData() + sizeof(int)*2, sizeof(int));

        WriteBatch *pbatch = new WriteBatch(3);

        pbatch -> put(k1, k1);
        pbatch -> put(k2, k2);
        pbatch -> put(k3, k3);

        return pbatch;
    }
};

TEST(A, Test)
{
    WriteBatch *pbatch = Test1();

    WriteBatch::Iterator iterator(pbatch);
    typedef pair<Slice, Slice> Node;

    for(const Node * node = iterator.first(); node != iterator.end(); node = iterator.next())
    {
        Slice a = node -> first;
        Slice b = node -> second;
        ASSERT_EQ(a.returnAsInt(), b.returnAsInt());
    }

    delete pbatch;

}
int main()
{
    RunAllTests();
    return 0;
}
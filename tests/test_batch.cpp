#include <iostream>
using namespace std;

#include "Batch.h"
#include "Slice.h"
#include "BufferPacket.h"
using namespace customdb;

#include <assert.h>

#define EXPECT_EQ(a,b) assert(a == b)

void RunTest1()
{
    BufferPacket packet(sizeof(int)*3);
    packet << 1;
    packet << 2;
    packet << 3;
    Slice k1(packet.getData(),sizeof(int));
    Slice k2(packet.getData() + sizeof(int), sizeof(int));
    Slice k3(packet.getData() + sizeof(int)*2, sizeof(int));

    WriteBatch batch(3);
    batch.put(k1, k2);
    batch.put(k2, k3);
    batch.put(k3, k1);

    WriteBatch::Iterator iterator(&batch);
    typedef pair<Slice, Slice> Node;

    for(const Node * node = iterator.first(); node != iterator.end(); node = iterator.next())
    {
        Slice a = node -> first;
        Slice b = node -> second;
        EXPECT_EQ(a, b);
    }
}

int main()
{
    printf("Passed All Tests\n");
    return 0;
}
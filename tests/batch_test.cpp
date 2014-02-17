#include <iostream>
using namespace std;

#include "BufferPacket.h"
#include "Batch.h"
#include "Slice.h"

int main()
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
        cout<<a.returnAsInt()<<" "<<b.returnAsInt()<<endl;
    }
    return 0;
}
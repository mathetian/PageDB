#include <iostream>
using namespace std;

#include "CustomDB.h"
#include "Option.h"
#include "TimeStamp.h"
#include "Log.h"

#include <assert.h>

#define SIZE 10000

int main()
{
    Options option;
    option.logOption.logLevel = LOG_ERROR;

    CustomDB * db = new CustomDB;
    db -> open(option);
    printf("open successful\n");

    TimeStamp::StartTime();

    for(int i=SIZE; i>=1; i--)
    {
        BufferPacket packet(sizeof(int));
        packet << i;
        Slice key(packet.getData(),sizeof(int));
        Slice value(packet.getData(),sizeof(int));
        if(db -> put(key, value) == false)
            cout << "error put:" << i << endl;
    }

    for(int i=1; i <= 100; i++)
    {
        BufferPacket packet(sizeof(int));
        packet << i;
        Slice key(packet.getData(),sizeof(int));
        assert(db -> remove(key) == true);
    }

    for(int i=100; i >= 1; i--)
    {
        BufferPacket packet(sizeof(int));
        packet << i;
        Slice key(packet.getData(),sizeof(int));
        Slice slice = db -> get(key);
        assert(slice.size() == 0);
    }

    for(int i=101; i<=SIZE; i++)
    {
        BufferPacket packet(sizeof(int));
        packet << i;
        Slice key(packet.getData(),sizeof(int));
        Slice slice = db -> get(key);
        assert(slice == key);
    }

}
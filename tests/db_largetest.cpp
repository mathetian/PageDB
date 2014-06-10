#include <iostream>
using namespace std;

#include "CustomDB.h"
#include "Option.h"
#include "TickTimer.h"
#include "BufferPacket.h"
using namespace customdb;

#include "TestUtils.h"
using namespace utils;

#define SIZE 1000000

class A { };

TEST(A, Test1)
{
    Options option;
    option.logOption.disabled = true;
    option.logOption.logLevel = LOG_FATAL;

    CustomDB * db = new CustomDB;
    TimeStamp ts;

    {
        db -> open(option);
        printf("open successful for put\n");

        ts.StartTime();

        for(int i=1; i<=SIZE; i++)
        {
            BufferPacket packet(sizeof(int));
            packet << i;

            if(db -> put(Slice(packet.c_str(),sizeof(int)),
                         Slice(packet.c_str(),sizeof(int))) == false)
                cout << "error put:" << i << endl;
            if(i%(SIZE/10)==0) cout<<"Put:"<<i<<endl;
        }

        ts.StopTime("PutTime: ");

        db -> close();
    }

    {
        db -> open(option);
        printf("open successful for put\n");

        ts.StartTime();

        for(int i=1; i<=SIZE; i++)
        {
            BufferPacket packet(sizeof(int));
            packet << i;

            Slice key(packet.c_str(), sizeof(int));

            Slice value = db -> get(key);

            BufferPacket packet2(sizeof(int));

            packet2 << value;
            packet2.reset();

            int num = -1;
            packet2 >> num;

            ASSERT_EQ(i, num);
        }

        ts.StopTime("GetTime(Without Cache): ");

        db -> close();
    }

    delete db;
}

TEST(A, Test2)
{
    Options option;
    option.logOption.disabled = true;
    option.logOption.logLevel = LOG_FATAL;

    CustomDB * db = new CustomDB;
    TimeStamp ts;

    {
        db -> open(option);
        printf("open successful for RunTest2 in compact\n");
        db -> compact();

        db -> close();
    }

    {
        db -> open(option);
        printf("open successful for Get Test after Compact\n");

        ts.StartTime();

        for(int i=1; i<=SIZE; i++)
        {
            BufferPacket packet(sizeof(int));
            packet << i;

            Slice key(packet.c_str(), sizeof(int));

            Slice value = db -> get(key);

            BufferPacket packet2(sizeof(int));

            packet2 << value;
            packet2.reset();

            int num = -1;
            packet2 >> num;

            ASSERT_EQ(i, num);
        }

        ts.StopTime("GetTime(Without Cache): ");

        db -> close();
        db -> destoryDB("demo");
    }
}

int main()
{
    RunAllTests();
    return 0;
}

#include <iostream>
using namespace std;

#include "CustomDB.h"
#include "Option.h"
#include "TimeStamp.h"

#include <assert.h>

#define SIZE 1000000

#define EXPECT_EQ(a,b) assert(a == b)
#define EXPECT_EQ_S(a,b) assert(strcmp(a,b) == 1)

void RunTest1()
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

        for(int i=1;i<=SIZE;i++)
        {
            BufferPacket packet(sizeof(int));
            packet << i;
            Slice key(packet.getData(),sizeof(int));
            Slice value(packet.getData(),sizeof(int));
            
            if(db -> put(key, value) == false) 
                cout << "error put:" << i << endl;
            if(i%(SIZE/100)==0) cout<<"Put:"<<i<<endl;
        }

        ts.StopTime("PutTime: ");

        db -> close();
    }

    {
        db -> open(option);
        printf("open successful for put\n");

        ts.StartTime();

        for(int i=1;i<=SIZE;i++)
        {
            BufferPacket packet(sizeof(int)); packet << i;
            
            Slice key(packet.getData(), sizeof(int));

            Slice value = db -> get(key);

            BufferPacket packet2(sizeof(int));
            
            packet2 << value; packet2.setBeg();
            
            int num = -1; packet2 >> num;
            if(i!=num)
                cout << i <<" "<<num <<" ) ";
            
            EXPECT_EQ(i, num);
        }

        ts.StopTime("GetTime(Without Cache): ");

        db -> close();
    }

    delete db;
}

void RunTest2()
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

        for(int i=1;i<=SIZE;i++)
        {
            BufferPacket packet(sizeof(int)); packet << i;
            
            Slice key(packet.getData(), sizeof(int));

            Slice value = db -> get(key);

            BufferPacket packet2(sizeof(int));
            
            packet2 << value; packet2.setBeg();
            
            int num = -1; packet2 >> num;
            if(i!=num)
                cout << i <<" "<<num <<" ) ";
            
            EXPECT_EQ(i, num);
        }

        ts.StopTime("GetTime(Without Cache): ");

        db -> close();
    }
}

int main()
{
    RunTest1();
    printf("Passed All Tests\n");
    return 0;
}
    
#include <iostream>
using namespace std;

#include "CustomDB.h"
#include "Option.h"
#include "TickTimer.h"
#include "BufferPacket.h"
using namespace customdb;

#include "TestUtils.h"
using namespace utils;

/**
	Create four thread, each put 250000 items.
	For each thread, each time, it compute 5000 items.
**/
#define SIZE 1000000
#define BATCHSIZE 250000
#define SUBSIZE   10000
#define THRNUM    4

class A { };

Options option;
CustomDB * db;

void* thrFunc(void * data)
{
    int thrid = *(int*)data;

    const int beg  = thrid*BATCHSIZE;
    const int round = (BATCHSIZE + SUBSIZE - 1)/SUBSIZE;

    TimeStamp thrtime;
    char buf[50];

    printf("thread %d begin\n", thrid);

    thrtime.StartTime();
    for(int i = 0; i < round; i++)
    {
        WriteBatch batch(SUBSIZE);

        for(int j = 0; j < SUBSIZE; j++)
        {
            int k = beg + i*SUBSIZE + j;

            BufferPacket packet(sizeof(int));
            packet << k;
            Slice key(packet.c_str(),sizeof(int));
            Slice value(packet.c_str(),sizeof(int));
            batch.put(key, value);
        }
        db -> tWrite(&batch);
        printf("thread %d finished round %d\n", thrid, i);
    }

    sprintf(buf, "Thread %d has been completed, spend time :", thrid);
    thrtime.StopTime(buf);

    return NULL;
}

/**
    Test for Batch-Digest
**/
TEST(A, Test1)
{
    option.logOption.disabled = true;
    option.logOption.logLevel = LOG_FATAL;

    db = new CustomDB;
    TimeStamp total;

    {
        db -> open(option);
        printf("open successful\n");

        int ids[THRNUM];
        Thread thrs[THRNUM];

        total.StartTime();

        for(int i = 0; i < THRNUM; i++)
        {
            ids[i]  = i;
            thrs[i] = Thread(thrFunc, &ids[i]);
            thrs[i].run();
        }

        for(int i = 0; i < THRNUM; i++) thrs[i].join();

        total.StopTime("Total PutTime(Thread Version): ");

        db -> close();
    }

    {
        db -> open(option);
        printf("open successful\n");

        printf("Begin Check\n");

        total.StartTime();
        for(int i=0; i < SIZE; i++)
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

            ASSERT_EQ(i,num);
        }
        total.StopTime("GetTime(Without Cache): ");

        db -> close();
    }

    delete db;
}

/**
    Test for compact
**/
TEST(A, Test2)
{
    option.logOption.disabled = true;
    option.logOption.logLevel = LOG_FATAL;

    db = new CustomDB;
    TimeStamp total;

    {
        db -> open(option);
        printf("open successful, Test After compact\n");

        total.StartTime();

        db -> compact();

        total.StopTime("Compact time: ");

        db -> close();
    }

    {
        db -> open(option);
        printf("open successful\n");

        printf("Begin Check\n");

        total.StartTime();
        for(int i=0; i < SIZE; i++)
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

            ASSERT_EQ(i,num);
        }

        total.StopTime("GetTime(Without Cache): ");

        db -> close();
        db -> destoryDB("demo");
    }

    delete db;
}

int main()
{
    RunAllTests();
    return 0;
}

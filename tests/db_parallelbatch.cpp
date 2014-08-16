// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "Option.h"
#include "PageDB.h"
#include "BufferPacket.h"
using namespace pagedb;

#include "TickTimer.h"
#include "TestUtils.h"
using namespace utils;

#define SIZE      1000000
#define BATCHSIZE 250000
#define ROUNDSIZE 50000
#define THRNUM    4

/**
** db_parallelbatch: parallel put. Basic Test for internal sync
**/

/**
** Put 1 Million items into db
** key size and value size are both 4 bytes
** Estimate Size 10MB
** Four Threads, each put 250,000 items
**/

class A { };

Options option;
PageDB * db;

void* thrFunc(void * data)
{
    int thrID = *(int*)data;

    const int beg  = thrID*BATCHSIZE;
    const int round = (BATCHSIZE + ROUNDSIZE - 1)/ROUNDSIZE;

    Timer total;
    char buf[50];

    printf("thread %d begin\n", thrID);

    total.Start();

    for(int i = 0; i < round; i++)
    {
        WriteBatch batch(ROUNDSIZE);

        for(int j = 0; j < ROUNDSIZE; j++)
        {
            int k = beg + i*ROUNDSIZE + j;

            BufferPacket packet(sizeof(int));
            packet << k;
            Slice key(packet.c_str(),sizeof(int));
            Slice value(packet.c_str(),sizeof(int));
            batch.put(key, value);
        }

        db -> put(&batch);

        printf("thread %d finished round %d\n", thrID, i);
    }

    sprintf(buf, "Thread %d has been completed, spend time :", thrID);

    total.Stop();
    total.Print(buf);

    return NULL;
}

TEST(A, Test1)
{
    option.logOption.disabled = true;
    option.logOption.logLevel = Log::LOG_FATAL;

    db = new PageDB;

    Timer total;

    {
        db -> open(option);
        printf("open successful\n");

        int      thrIDS[THRNUM];
        Thread **thrEDS = new Thread*[THRNUM];

        total.Start();

        for(int i = 0; i < THRNUM; i++)
        {
            thrIDS[i]  = i;
            thrEDS[i] = new Thread(thrFunc, &thrIDS[i]);
            thrEDS[i] -> run();
        }

        for(int i = 0; i < THRNUM; i++) thrEDS[i] -> join();

        total.Stop();
        total.Print("Total PutTime(Thread Version): ");

        db -> close();
    }

    {
        db -> open(option);
        printf("open successful\n");

        printf("Begin Check\n");

        total.Start();

        for(int i=0; i < BATCHSIZE*THRNUM; i++)
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

        total.Stop();
        total.Print("GetTime(Without Cache): ");

        db -> close();
    }

    delete db;
}

TEST(A, Test2)
{
    option.logOption.disabled = true;
    option.logOption.logLevel = Log::LOG_FATAL;

    db = new PageDB;
    Timer total;

    {
        db -> open(option);
        printf("open successful, Test After compact\n");

        total.Start();

        // db -> dump(cout);
        db -> compact();

        total.Stop();
        total.Print("Compact time: ");

        db -> close();
    }

    {
        db -> open(option);
        printf("open successful\n");

        printf("Begin Check\n");

        total.Start();

        for(int i=0; i < BATCHSIZE*THRNUM; i++)
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

        total.Stop();
        total.Print("GetTime(Without Cache): ");

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

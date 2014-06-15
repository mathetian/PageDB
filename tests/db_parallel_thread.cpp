// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "TestUtils.h"
using namespace utils;

#include "Option.h"
#include "CustomDB.h"
#include "TickTimer.h"
#include "BufferPacket.h"
using namespace customdb;

/**
** db_parallel_thread: parallel put. Basic Test for internal sync
**/

/**
** Put 1 Million items into db
** key size and value size are both 4 bytes
** Estimate Size 10MB
** Four Threads, each put 250,000 items
**/
#define SIZE      1000000
#define BATCHSIZE 250000
#define SUBSIZE   50000
#define THRNUM    4

Options option;
CustomDB * db;

class A { };

void* thrFunc(void * data)
{
    int thrid = *(int*)data;

    const int beg  = thrid*BATCHSIZE;
    const int round = (BATCHSIZE + SUBSIZE - 1)/SUBSIZE;

    Timer thrtime;
    char buf[50];

    printf("thread %d begin\n", thrid);

    thrtime.Start();

    for(int i = 0; i < round; i++)
    {
        WriteBatch batch(SUBSIZE);

        for(int j = 0; j < SUBSIZE; j++)
        {
            int k = beg + i*SUBSIZE + j;

            BufferPacket packet(sizeof(int));
            packet << k;

            batch.put(Slice(packet.c_str(),sizeof(int)), Slice(packet.c_str(),sizeof(int)));
        }

        db -> put(&batch);

        printf("thread %d finished round %d\n", thrid, i);
    }

    sprintf(buf, "Thread %d has been completed, spend time :", thrid);
    thrtime.Stop();
    thrtime.PrintElapsedTime(buf);

    return NULL;
}

/**
    Test for Batch-Digest
**/
TEST(A, Test1)
{
    option.logOption.disabled = true;
    option.logOption.logLevel = Log::LOG_FATAL;

    db = new CustomDB;
    Timer total;

    {
        db -> open(option);
        printf("open successful\n");

        int    thrIDS[THRNUM];
        Thread **thrEDS = new Thread[THRNUM];

        total.Start();

        for(int i = 0; i < THRNUM; i++)
        {
            thrIDS[i]  = i;
            thrEDS[i]  = new Thread(thrFunc, &thrIDS[i]);
            thrEDS[i] -> run();
        }

        for(int i = 0; i < THRNUM; i++) thrEDS[i] -> join();

        total.Stop();
        total.PrintElapsedTime("Total PutTime(Thread Version): ");

        db -> close();
    }

    int freq = 0;

    {
        db -> open(option);
        printf("open successful\n");

        printf("Begin Check\n");

        total.Start();
        
        for(int i = 0; i < SIZE; i++)
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

            if(i != num) freq++;
        }
        
        total.Stop();
        total.PrintElapsedTime("GetTime(Without Cache): ");
        
        db -> close();
        db -> destoryDB("demo");
    }

    cout<<freq<<endl;
    delete db;
}

int main()
{
    RunAllTests();
    return 0;
}

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

#define SIZE 1000000

class A { };

TEST(A, Test1)
{
    Options option;
    option.logOption.disabled = true;
    option.logOption.logLevel = Log::LOG_FATAL;

    PageDB * db = new PageDB;
    Timer ts;

    {
        db -> open(option);
        printf("open successful for put\n");

        ts.Start();

        for(int i=1; i<=SIZE; i++) {
            BufferPacket packet(sizeof(int));
            packet << i;

            if(db -> put(Slice(packet.c_str(),sizeof(int)),
                         Slice(packet.c_str(),sizeof(int))) == false)
                cout << "error put:" << i << endl;
            if(i%(SIZE/10)==0) cout<<"Put:"<<i<<endl;
        }

        ts.Stop();
        ts.Print("PutTime: ");

        db -> close();
    }

    {
        db -> open(option);
        printf("open successful for get\n");

        ts.Start();

        for(int i=1; i<=SIZE; i++) {
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

        ts.Stop();
        ts.Print("GetTime(Without Cache): ");

        db -> close();
    }

    delete db;
}

TEST(A, Test2)
{
    Options option;
    option.logOption.disabled = true;
    option.logOption.logLevel = Log::LOG_FATAL;

    PageDB * db = new PageDB;
    Timer ts;

    {
        db -> open(option);
        printf("open successful for RunTest2 in compact\n");
        db -> compact();

        db -> close();
    }

    {
        db -> open(option);
        printf("open successful for Get Test after Compact\n");

        ts.Start();

        for(int i=1; i<=SIZE; i++) {
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

        ts.Stop();
        ts.Print("GetTime(Without Cache): ");

        db -> close();
        db -> destoryDB("demo");
    }
}

int main()
{
    RunAllTests();
    return 0;
}

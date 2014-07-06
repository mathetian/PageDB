// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "Option.h"
#include "CustomDB.h"
#include "BufferPacket.h"
using namespace customdb;

#include "TickTimer.h"
#include "TestUtils.h"
using namespace utils;

#define SIZE      1000000
#define BATCHSIZE 10000

class A { };

TEST(A, Test1)
{
    Options option;
    char str[256];
    CustomDB * db = new CustomDB;

    Timer total, part;

    {
        db -> open(option);
        printf("open successful\n");

        int round = (SIZE + BATCHSIZE - 1)/BATCHSIZE;

        total.Start();
        for(int i = 0; i < round; i++) {
            part.Start();

            WriteBatch batch(BATCHSIZE);

            for(int j = 0; j < BATCHSIZE; j++) {
                int k = i*BATCHSIZE + j;
                if(k >= SIZE) break;

                BufferPacket packet(sizeof(int));
                packet << k;
                Slice key(packet.c_str(),sizeof(int));
                Slice value(packet.c_str(),sizeof(int));
                batch.put(key, value);
            }

            db -> put(&batch);

            batch.clear();
            sprintf(str, "In round %d, PutTime: ", i);

            part.Stop();
            part.Print(str);
        }

        sprintf(str, "Total PutTime: ");

        total.Stop();
        total.Print(str);

        db -> close();
    }

    {
        db -> open(option);
        printf("open successful\n");

        total.Start();

        for(int i=SIZE-1; i>=0; i--) {
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
        db -> destoryDB("demo");
    }

    delete db;
}

int main()
{
    RunAllTests();
    return 0;
}
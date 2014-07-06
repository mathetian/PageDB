// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "Batch.h"
#include "BufferPacket.h"
using namespace customdb;

#include "Slice.h"
#include "TestUtils.h"
using namespace utils;

#include "CommonHeader.h"

class A
{
public:
    WriteBatch* Test() {
        BufferPacket packet(sizeof(int)*3);
        packet << 1;
        packet << 2;
        packet << 3;

        Slice k1(packet.c_str(), sizeof(int));
        Slice k2(packet.c_str() + sizeof(int), sizeof(int));
        Slice k3(packet.c_str() + sizeof(int)*2, sizeof(int));

        WriteBatch *pbatch = new WriteBatch(3);

        pbatch -> put(k1, k1);
        pbatch -> put(k2, k2);
        pbatch -> put(k3, k3);

        return pbatch;
    }
};

TEST(A, Test)
{
    WriteBatch *pbatch = Test();

    WriteBatch::Iterator iterator(pbatch);
    typedef pair<Slice, Slice> Node;

    for(const Node * node = iterator.first(); node != iterator.end(); node = iterator.next()) {
        Slice a = node -> first;
        Slice b = node -> second;
        ASSERT_EQ(a.returnAsInt(), b.returnAsInt());
    }

    delete pbatch;

}
int main()
{
    RunAllTests();
    return 0;
}
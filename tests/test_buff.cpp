// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "TestUtils.h"
using namespace utils;

#include "BufferPacket.h"
using namespace customdb;

class A { };

TEST(A, TestSame)
{
    BufferPacket packet1(10);
    packet1 << "hello";

    BufferPacket packet2(10);
    packet2 << "hello";

    ASSERT_NE(packet1, packet2);

    packet2 = packet1;

    ASSERT_EQ(packet1, packet2);
}

int main()
{
    RunAllTests();
    return 0;
}
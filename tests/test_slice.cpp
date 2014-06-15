// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "BufferPacket.h"
using namespace customdb;

#include "Slice.h"
#include "TestUtils.h"
using namespace utils;

class A
{
public:
    int GetInt()
    {
        BufferPacket packet(sizeof(int));
        packet << 15545;
        Slice slice(packet.c_str(), packet.size());
        return slice.returnAsInt();
    }

    int GetSize1()
    {
        Slice slice;
        return slice.size();
    }

    int GetSize2()
    {
        vector<Slice> slices = vector<Slice>(10,0);
        return slices.size();
    }
};

/**
** Test Binary representation
**/
TEST(A, Test1)
{
    ASSERT_EQ(GetInt(), 15545);
}

/**
** Test Size
**/
TEST(A, Test2)
{
    ASSERT_EQ(GetSize1(), 0);
    ASSERT_EQ(GetSize2(), 10);
}

/**
** Test for String Comparation
**/
TEST(A, Test3)
{
    ASSERT_NE(Slice("Hello"), "Hello1");
}

TEST(A, Test4)
{
    Slice slice;
   // slice = Slice("hello");
    Slice slice2("hello");
}

int main()
{
    RunAllTests();
    return 0;
}
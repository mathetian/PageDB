// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "TestUtils.h"
using namespace utils;

#include "Slice.h"
#include "Option.h"
#include "CustomDB.h"
using namespace customdb;

/**
** db_smalltest: For put/get/compact
**/
class A { };

/**Test for put and get**/
TEST(A, Test1)
{
    Options option;
    CustomDB * db = new CustomDB;

    {
        db -> open(option);
        printf("open successful for Test1 in put\n");

        db -> put("hello","world");
        db -> put("hello1","world1");
        db -> put("hello12","world123");
        db -> close();
    }

    {
        db -> open(option);
        printf("open successful for Test1 in get\n");

        ASSERT_EQ(db -> get("hello"), Slice("world"));
        ASSERT_EQ(db -> get("hello1"), Slice("world1"));
        ASSERT_EQ(db -> get("hello12"), Slice("world123"));

        db -> close();
    }

    delete db;
}

/**Test for compact, failed passed this Test**/
TEST(A, Test2)
{
    Options option;
    CustomDB * db = new CustomDB;

    {
        db -> open(option);
        printf("open successful for RunTest2 in get\n");

        ASSERT_EQ(db -> get("hello"), Slice("world"));
        ASSERT_EQ(db -> get("hello1"), Slice("world1"));
        ASSERT_EQ(db -> get("hello12"), Slice("world123"));

        db -> close();
    }

    {
        db -> open(option);
        printf("open successful for RunTest2 in compact\n");
        db -> compact();

        db -> close();
    }

    {
        db -> open(option);
        printf("open successful for RunTest2 in test After compact\n");

        ASSERT_EQ(db -> get("hello"), Slice("world"));
        ASSERT_EQ(db -> get("hello1"), Slice("world1"));
        ASSERT_EQ(db -> get("hello12"), Slice("world123"));

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
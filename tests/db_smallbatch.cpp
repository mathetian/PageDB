// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "TestUtils.h"
using namespace utils;

#include "Batch.h"
#include "Option.h"
#include "CustomDB.h"
using namespace customdb;

/**
** db_smallbatch: put(2)/get
**/

class A { };

TEST(A, Test1)
{
    Options option;

    CustomDB * db = new CustomDB;

    {
        db -> open(option);
        printf("open successful\n");

        WriteBatch batch;
        batch.put("hello","world");
        batch.put("hello1","world1");
        batch.put("hello12","world123");

        db -> put(&batch);
        db -> close();
    }

    {
        db -> open(option);
        printf("open successful\n");

        ASSERT_EQ(db->get("hello"), "world");
        ASSERT_EQ(db->get("hello1"), "world1");
        ASSERT_EQ(db->get("hello12"), "world123");

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
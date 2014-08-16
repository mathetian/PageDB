// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "TestUtils.h"
using namespace utils;

/**
** This file is used to test two features of TestUtils
**/
class A
{
public:
    int func1()
    {
        return 1;
    }

    int func2()
    {
        return 2;
    }
};

TEST(A, abc1)
{
    ASSERT_TRUE(func1() == 1);
}

TEST(A, abc2)
{
    ASSERT_TRUE(func2() == 2);
}

int main()
{
    RunAllTests();
    return 0;
}
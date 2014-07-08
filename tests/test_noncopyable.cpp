// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "TestUtils.h"
#include "Noncopyable.h"
using namespace utils;

class A : Noncopyable
{
};

TEST(A, Test1)
{
    A a;
    //A b = a;
    //A c(a);
    A d;
    //d = a;
}

int main()
{
    RunAllTests();
    return 0;
}
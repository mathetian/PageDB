// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "TestUtils.h"
#include "FileModule.h"
using namespace utils;

class A { };

TEST(A, Test1)
{
    ASSERT_EQ(FileModule::Exist("Makefile"), true);
    ASSERT_EQ(FileModule::Exist("Makefile1"), false);

    ASSERT_NE(FileModule::Size("Makefile"), 0);
    ASSERT_EQ(FileModule::Size("Makefile2"), 0);
}

TEST(A, Test2)
{
    RandomFile file;
    file.Open("Hello.txt");
    string str = "123213\n";
    file.Write("123213\n", 0, str.size());

    file.Close();
}

int main()
{
    RunAllTests();
    return 0;
}
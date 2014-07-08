// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "TickTimer.h"
#include "TestUtils.h"
using namespace utils;

/**
** This file is used to test TickTimer
**/

class A { };

/**
** Expect Output: The second value is almost same as the first one.
**/
TEST(A, Timer)
{
    Timer timer;
    timer.Start();
    sleep(1);
    timer.Stop();
    timer.Print("Timer Test1");

    timer.Start();
    sleep(1);
    timer.Stop();
    timer.Print("Timer Test2");
}

/**
** Expect Output: The second value is twice bigger than the first one
**/
TEST(A, TimeAccumulator)
{
    TimeAccumulator accumlator;
    accumlator.StartTimer();
    sleep(1);
    accumlator.StopTimer();
    accumlator.Print("TimeAccumulator Test1");

    accumlator.StartTimer();
    sleep(1);
    accumlator.StopTimer();
    accumlator.Print("TimeAccumulator Test2");
}

int main()
{
    RunAllTests();
    return 0;
}
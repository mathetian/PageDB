// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "TestUtils.h"
#include "Multithreading.h"
using namespace utils;

class A { };

RWLock rwlock;

void * func1(void *)
{
    rwlock.readLock();
    cout << "Expect 1" << endl;
    rwlock.readUnlock();
    sleep(1);
    rwlock.writeLock();
    cout << "Expect 3" << endl;
    rwlock.writeUnlock();
}

void * func2(void *)
{
    rwlock.readLock();
    sleep(2);
    cout << "Expect 2" << endl;
    rwlock.readUnlock();
}

void * func3(void *)
{
    rwlock.writeLock();
    cout << "Expect 1" << endl;
    sleep(5);
    rwlock.writeUnlock();
    sleep(1);
    rwlock.writeLock();
    cout << "Expect 3" << endl;
    rwlock.writeUnlock();
}

void * func4(void *)
{
    rwlock.readLock();
    cout << "Expect 2" << endl;
    sleep(3);
    rwlock.readUnlock();
}

/**
** Read Then Write
**/
TEST(A, RRW)
{
    Thread thr1(func1, NULL);
    Thread thr2(func2, NULL);

    thr1.run();
    thr2.run();
    thr1.join();
    thr2.join();
}

/**
** Write then Read
**/
TEST(A, WWR)
{
    Thread thr1(func3, NULL);
    Thread thr2(func4, NULL);

    thr1.run();
    thr2.run();
    thr1.join();
    thr2.join();
}

int main()
{
    RunAllTests();
    return 0;
}
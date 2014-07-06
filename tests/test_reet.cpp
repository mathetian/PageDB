// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "TestUtils.h"
#include "Multithreading.h"
using namespace utils;

/**
** Unit Test for ReentrantLock
**/

class CA
{
public:
    void A() {
        m_lock.lock();
        cout << "Function A " << Thread::getIDType() << endl;
        B();
        m_lock.unlock();
    }

    void B() {
        m_lock.lock();
        cout << "Function B " << Thread::getIDType() << endl;
        sleep(10);
        m_lock.unlock();
    }

private:
    ReentrantLock m_lock;
};

CA ca;

void *invoke_reentrantLock(void*)
{
    ca.A();
}

/**
** Expect output: same Thread_ID twice and then another Thread_ID
**/
TEST(CA, ReentrantLock)
{
    int a1 = 0, b1 = 1;
    Thread thr1(invoke_reentrantLock, NULL);
    Thread thr2(invoke_reentrantLock, NULL);

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
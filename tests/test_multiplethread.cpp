// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "TestUtils.h"
#include "Multithreading.h"
using namespace utils;

/**
** Unit Test for Thread/SingletonMutex
**/

class A
{
public:
    A() : m_mutex(SingletonMutex::getInstance()) { }

    void a() {
        sleep(1);
        cout << "Expect 2" << endl;
        sleep(1);
        cout << "Expect 4" << endl;
    }

    void b() {
        cout << "Expect 1" << endl;
        sleep(1);
        cout << "Expect 3" << endl;
    }

    void c() {
        ScopeMutex scope(m_mutex.RMutex());
        cout << "Expect 1" << endl;
        sleep(2);
        cout << "Expect 2" << endl;
    }

    void d() {
        ScopeMutex scope(m_mutex.RMutex());
        cout << "Expect 3" << endl;
        sleep(2);
        cout << "Expect 4" << endl;
    }

public:
    SingletonMutex &m_mutex;
};

A aa, bb;

void *invoke_ab(void *data)
{
    int flag = *(int*)data;
    if(flag == 0) aa.a();
    else aa.b();
}

void *invode_cd(void *data)
{
    int flag = *(int*)data;
    if(flag == 0) aa.c();
    else aa.d();
}

void *invoke_cd_aa_bb(void *data)
{
    int flag = *(int*)data;
    if(flag == 0) aa.c();
    else bb.d();
}

/**
** Epect output: from 1 to 4
**/
TEST(A, Thread)
{
    int a1 = 0, b1 = 1;
    Thread thr1(invoke_ab, &a1);
    Thread thr2(invoke_ab, &b1);

    thr1.run();
    thr2.run();
    thr1.join();
    thr2.join();
}

/**
** Epect output: from 1 to 4
**/
TEST(A, ScopeMutex)
{
    int a1 = 0, b1 = 1;
    Thread thr1(invode_cd, &a1);
    Thread thr2(invode_cd, &b1);

    thr1.run();
    thr2.run();
    thr1.join();
    thr2.join();
}

/**
** Epect output: 12, 34 or 34, 12
**/
TEST(A, SingletoMutex)
{
    int a1 = 0, b1 = 1;
    Thread thr1(invoke_cd_aa_bb, &a1);
    Thread thr2(invoke_cd_aa_bb, &b1);

    thr1.run();
    thr2.run();
    thr1.join();
    thr2.join();

    assert(&aa.m_mutex == &bb.m_mutex);
}

int main()
{
    RunAllTests();
    return 0;
}
#include "Thread.h"
using namespace utils;

#include <unistd.h>
#include <stdio.h>
#include <assert.h>

class A
{
public:
    A() : m_mutex(SingletonMutex::getInstance()) { }

    void a()
    {
        sleep(1);
        printf("a1\n");
        sleep(1);
        printf("a2\n");
    }

    void b()
    {
        printf("b1\n");
        sleep(1);
        printf("b2\n");
    }

    void c()
    {
        ScopeMutex scope(&m_mutex.m);
        printf("c1\n");
        sleep(2);
        printf("c2\n");
    }

    void d()
    {
        ScopeMutex scope(&m_mutex.m);
        printf("d1\n");
        sleep(2);
        printf("d2\n");
    }

public:
    /**
    	Can't be static, why?
    	Maybe can use singleton.
    **/
    SingletonMutex &m_mutex;
};

A aa, bb;

void *ffff(void *data)
{
    int flag = *(int*)data;
    if(flag == 0) aa.a();
    else aa.b();
}

void *ffff2(void *data)
{
    int flag = *(int*)data;
    if(flag == 0) aa.c();
    else aa.d();
}

void *ffff3(void *data)
{
    int flag = *(int*)data;
    if(flag == 0) aa.c();
    else bb.d();
}

/**
	Basic test to testify whether ThreadPosix is proper.
	Also shows the `global` access.
**/
void RunTest1()
{
    int a1 = 0, b1 = 1;
    Thread thr1(ffff, &a1);
    Thread thr2(ffff, &b1);

    thr1.run();
    thr2.run();
    thr1.join();
    thr2.join();
}

/**
	Mutex and ScopeMutex test
**/
void RunTest2()
{
    int a1 = 0, b1 = 1;
    Thread thr1(ffff2, &a1);
    Thread thr2(ffff2, &b1);

    thr1.run();
    thr2.run();
    thr1.join();
    thr2.join();
}

/**
	Test for whether mutex will be shared in all different instance of class.
	The answer is no for sure.
**/
void RunTest3()
{
    int a1 = 0, b1 = 1;
    Thread thr1(ffff3, &a1);
    Thread thr2(ffff3, &b1);

    thr1.run();
    thr2.run();
    thr1.join();
    thr2.join();

    assert(&aa.m_mutex == &bb.m_mutex);
}

int main()
{
    RunTest1();
    printf("Passed Test1\n");
    RunTest2();
    printf("Passed Test2\n");
    RunTest3();
    printf("Passed Test3\n");
    
    printf("Passed All Tests\n");
    return 0;
}
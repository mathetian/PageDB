#include "../helpers/Thread.h"
#include <unistd.h>
#include <stdio.h>
#include <assert.h>


Mutex mutex;
RWLock rwlock(&mutex);

void * func1(void *)
{
    rwlock.readLock();
    printf("123\n");
    rwlock.readUnlock();
    sleep(1);
    rwlock.writeLock();
    printf("567\n");
}

void * func2(void *)
{
    rwlock.readLock();
    printf("456\n");
    sleep(2);
    rwlock.readUnlock();
}

void * func3(void *)
{
    rwlock.writeLock();
    printf("123\n");
    sleep(5);
    rwlock.writeUnlock();
    //sleep(1);
    rwlock.writeLock();
    printf("567\n");
    rwlock.writeUnlock();
}

void * func4(void *)
{
    sleep(1);
    rwlock.readLock();
    printf("456\n");
    sleep(3);
    rwlock.readUnlock();
}

/**Test for read-reentrant and write after read-lock**/
void RunTest1()
{
    Thread thr1(func1, NULL);
    Thread thr2(func2, NULL);

    thr1.run();
    thr2.run();
    thr1.join();
    thr2.join();
}

/**Test for read after write(of course, can't read util writeunlock**/
void RunTest2()
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
    RunTest2();
    return 0;
}
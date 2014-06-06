// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _THREAD_H
#define _THREAD_H

#include "CommonHeader.h"

#include "Noncopyable.h"

/**
** Thread.h is the archive of many sub-module of concurrency.
**
** Support: Thread/Mutex/CondVar/SingletonMutex/ScopeMutex/RWLock/ReentrantLock
**/
namespace utils
{

typedef pthread_t id_type;
typedef void *(*Task)(void *);
typedef pthread_mutex_t mutex_type;

class Thread : Noncopyable
{
public:
    Thread(Task task, void * args = NULL) 
        : m_task(task), m_args(args), m_tid(-1) 
    { 

    }
    
public:
    /**
    ** Forbid being called more than once
    **/
    id_type  run()
    {
        assert(m_tid == -1);
        pthread_create(&m_tid, NULL, m_task, m_args);
        return m_tid;
    }
    /**
    ** Only can be called after `run`
    **/
    void     join()
    {
       
        assert(m_tid != -1);
        pthread_join(m_tid, NULL);
    }
    /**
    ** Only can be called after `run`
    **/
    void     cancel()
    {
        assert(m_tid != -1);
        pthread_cancel(m_tid);
    }

private:
    id_type  m_tid;
    Task     m_task;
    void    *m_args;
};

class Mutex : Noncopyable
{
public:
    Mutex()
    {
        pthread_mutex_init(&m_mutex, 0);
    }

    ~Mutex()
    {
        pthread_mutex_destroy(&m_mutex);
    }

public:
    void lock()
    {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }

    bool  trylock()
    {
        return pthread_mutex_trylock(&m_mutex);
    }   

public:
    mutex_type& getMutex()
    {
        return m_mutex;
    }

private:
    mutex_type   m_mutex;
};

class CondVar : Noncopyable
{
public:
    CondVar(Mutex* mutex = NULL): m_mutex(mutex)
    {
        pthread_cond_init(&m_cond, NULL);
    }

    ~CondVar()
    {
        pthread_cond_destroy(&m_cond);
    }

public:
    void wait()
    {
        pthread_cond_wait(&m_cond, &m_mutex->getMutex());
    }

    void signal()
    {
        pthread_cond_signal(&m_cond);
    }

    void signalAll()
    {
        pthread_cond_broadcast(&m_cond);
    }

private:
    pthread_cond_t m_cond;
    Mutex         *m_mutex;
};

/**
    Two ways for Singleton

    The first way, not thread_safe
    class Singleton{
    public:
        Singleton * getInstance()
        {
            if(s == NULL)
                s = new Singleton;
            return s;
        }
    private:
        Singleton() { }
        static Singleton * s;
    }

    Singleton * a = Singleton::getInstance();
    Singleton * b = Singleton::getInstance();
    assert(a == b);//judge by address

    For the way below,
    SingletonMutex &a = SingletonMutex::getInstance();
    SingletonMutex &b = SingletonMutex::getInstance();
    assert(&a == &b);
**/

/**
** SingletonMutex is for a single instance of mutex
**/
class SingletonMutex : Noncopyable
{
public:
    static SingletonMutex& getInstance()
    {
        static SingletonMutex instance;
        return instance;
    }

private:
    SingletonMutex() {}
    Mutex m_mutex;
};

/**
** A RAII Instance
**/
class ScopeMutex : Noncopyable
{
public:
    ScopeMutex(Mutex * pmutex)
        : m_pmutex(pmutex)
    {
        m_pmutex -> lock();
    }

    ~ScopeMutex()
    {
        m_pmutex -> unlock();
    }

private:
    Mutex * m_pmutex;
};

/**
** We implemented the RWLock with mutex and condition variable
**
** It can't be promoted.
**/
class RWLock : Noncopyable
{
public:
    RWLock(Mutex * mutex): m_mutex(mutex), m_condRead(m_mutex),
        m_condWrite(m_mutex), m_nReader(0), m_nWriter(0), m_wReader(0), m_wWriter(0)
    {
        m_mutex = new Mutex;
    }

public:
    void readLock()
    {
        ScopeMutex scope(m_mutex);
        if(m_nWriter || m_wWriter)
        {
            m_wReader++;
            while(m_nWriter || m_wWriter)
                m_condRead.wait();
            m_wReader--;
        }
        m_nReader++;
    }

    void readUnlock()
    {
        ScopeMutex scope(m_mutex);
        m_nReader--;

        if(m_wWriter != 0)
            m_condWrite.signal();
        else if(m_wReader != 0)
            m_condRead.signal();
    }

    void writeLock()
    {
        ScopeMutex scope(m_mutex);
        if(m_nReader || m_nWriter)
        {
            m_wWriter++;
            while(m_nReader || m_nWriter)
                m_condWrite.wait();
            m_wWriter--;
        }
        m_nWriter++;
    }
    
    void writeUnlock()
    {
        ScopeMutex scope(m_mutex);
        m_nWriter--;

        if(m_wWriter != 0)
            m_condWrite.signal();
        else if(m_wReader != 0)
            m_condRead.signal();
    }

private:
    int  m_nReader;
    int  m_nWriter;
    int  m_wReader;
    int  m_wWriter;

private:
    Mutex * m_mutex;
    CondVar m_condRead;
    CondVar m_condWrite;
};

/**
** ReentrantLock means that in single thread, it can be called more than once
**/
class ReentrantLock : Noncopyable
{
public:
    ReentrantLock() 
        : m_id(-1), m_cond(&m_tmplock), m_time(0) 
    { 

    }
    
    void  lock()
    {
        m_tmplock.lock();

        if(m_id == -1)
        {
            assert(m_lock.trylock() == 0);
            m_id = pthread_self();
            m_tmplock.unlock();
            m_time = 1;
            return;
        }
        else if(m_id == pthread_self())
        {
            m_tmplock.unlock();
            m_time++;
            return;
        }

        while(m_id != -1) m_cond.wait();
        m_id = pthread_self();
        m_time = 1;
        m_tmplock.unlock();
    }

    void  unlock()
    {
        m_tmplock.lock();

        m_time--;
        if(m_time == 0)
        {
            m_lock.unlock();
            m_id = -1;
            m_tmplock.unlock();
            m_cond.signal();
        }
        else
            m_tmplock.unlock();
    }

    bool  trylock()
    {
        m_tmplock.lock();
        if(m_id != -1)
        {
            m_tmplock.unlock();
            return false;
        }
        else
        {
            assert(m_lock.trylock() == 0);
            m_id = pthread_self();
            m_tmplock.unlock();
            m_time = 1;
            return true;
        }
    }

private:
    Mutex   m_lock;
    id_type m_id;
    Mutex   m_tmplock;
    CondVar m_cond;
    int     m_time;
};

};

#endif
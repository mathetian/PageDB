#include "Thread.h"
using namespace utils;

#include <assert.h>

namespace utils{

#ifdef __WIN32
#else
pthread_t Thread::run()
{
    if(m_tid != -1)
        return m_tid;

    pthread_create(&m_tid, NULL, m_task, m_args);
    return m_tid;
}

void  Thread::join()
{
    if(m_tid != -1)
        pthread_join(m_tid, NULL);
}

void  Thread::cancel()
{
    if(m_tid != -1)
        pthread_cancel(m_tid);
}

id_type Thread::getIDType()
{
    return pthread_self();
}

Mutex::Mutex()
{
    pthread_mutex_init(&m_mutex, 0);
}

Mutex::~Mutex()
{
    pthread_mutex_destroy(&m_mutex);
}

void Mutex::lock()
{
    pthread_mutex_lock(&m_mutex);
}

void Mutex::unlock()
{
    pthread_mutex_unlock(&m_mutex);
}

int Mutex::trylock()
{
    return pthread_mutex_trylock(&m_mutex);
}

CondVar::CondVar(Mutex* mutex) : m_mutex(mutex)
{
    pthread_cond_init(&m_cond, NULL);
}

CondVar::~CondVar()
{
    pthread_cond_destroy(&m_cond);
}

void CondVar::wait()
{
    pthread_cond_wait(&m_cond, &m_mutex->m_mutex);
}

void CondVar::signal()
{
    pthread_cond_signal(&m_cond);
}

void CondVar::signalAll()
{
    pthread_cond_broadcast(&m_cond);
}

void ReentrantLock::lock()
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

void ReentrantLock::unlock()
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

bool ReentrantLock::trylock()
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

};
#endif
#include "Thread.h"
using namespace utils;

#include <assert.h>
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

CondVar::CondVar(Mutex* mutex)
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

#endif
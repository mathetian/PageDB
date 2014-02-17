#include "Thread.h"

using namespace utils;

#ifdef __WIN32
ThreadID Thread::run()
{
    if(m_tid != NULL)  return m_tid;
    m_tid = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)m_task, m_args, 0, NULL);

    return m_tid;
}

void  Thread::join()
{
    if(m_tid != NULL)
        WaitForSingleObject(m_tid, INFINITE);
}

void  Thread::cancel()
{
    if(m_tid != NULL)
        TerminateThread(m_tid, 0);
}

Mutex::Mutex()
{
    InitializeCriticalSection(&m_mutex);
}

Mutex::~Mutex()
{

}

void Mutex::lock()
{
    EnterCriticalSection(&m_mutex);
}

void Mutex::unlock()
{
    LeaveCriticalSection(&m_mutex);
}

int Mutex::trylock()
{
    //Todo list
}

CondVar::CondVar(Mutex* mutex)
{
    m_cond = CreateEvent(NULL, TRUE, FALSE, TEXT("m_cond"));
}

CondVar::~CondVar()
{
}

void CondVar::wait()
{
    WaitForSingleObject(m_cond, INFINITE);
}

void CondVar::signal()
{
    SetEvent(m_cond);
}

void CondVar::signallAll()
{
    //Todo list
}
#endif
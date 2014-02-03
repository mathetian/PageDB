#ifndef _THREAD_POSIX_H
#define _THREAD_POSIX_H

#include <pthread.h>

class ThreadPosix{
public:
typedef void *(*Task)(void *);
public:
	ThreadPosix(Task task = NULL, void * args = NULL) : m_task(task), m_args(args) { }
	
	ThreadPosix(const ThreadPosix & thr) : m_task(thr.m_task), m_args(thr.m_args) { }

	pthread_t run() 
	{ 
		if(m_tid == -1) 
			return m_tid; 
	
		pthread_create(&m_tid, NULL, m_task, m_args); 
		return m_tid; 
	}

	void   join() { if(m_tid != -1) pthread_join(m_tid, NULL); }

	void   cancel() { if(m_tid != -1) pthread_cancel(m_tid); }
private:
	pthread_t m_tid;
	Task m_task;
	void * m_args;
};

class Mutex{
public:
  Mutex() { pthread_mutex_init(&m_mutex, 0); }
  ~Mutex() { pthread_mutex_destroy(&m_mutex); }
  void lock() { pthread_mutex_lock(&m_mutex);}
  void unlock() { pthread_mutex_unlock(&m_mutex); }
private: 
	pthread_mutex_t m_mutex;
};

class ScopeMutex{
public:
    ScopeMutex(Mutex * pmutex) : m_pmutex(pmutex) { m_pmutex -> lock(); }
    ~ScopeMutex() { m_pmutex -> unlock(); }
private: 
    Mutex * m_pmutex;
};

#endif
#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;

#include <boost/function.hpp>
using namespace boost;
/**
	Use C++0x thread, mutex, lock.
**/
class ThreadPool{
public:
	ThreadPool(int threadNum = 5);
   ~ThreadPool();

public:
	bool schedule()
	{
		lock_guard<recursive_mutex> lock(m_monitor);
	}
    int  active() const;
    int  pending() const;
    void clear();
    bool empty();
   	bool wait();
   	bool wait(struct timeval & spec) const;
   	bool resize(int threadNum = 0);

private:
    int m_threadNum;		
    int m_actThreadNum;
    recursive_mutex  m_monitor;
    condition_variable m_worker_idle_or_terminated_event;
    condition_variable m_task_or_terminate_workers_event;
    typedef function0<void> Task;
};
#endif
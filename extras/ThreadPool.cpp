#include "ThreadPool.h"

ThreadPool::ThreadPool(int threadNum)
{
	m_actThreadNum = 0;
	m_threadNum    = 0;
	m_targetNum    = 0;
	resize(threadNum);
}

ThreadPool::~ThreadPool()
{
	killAllThreads();
	
	m_actThreadNum = 0;
	m_threadNum = 0;
}

void ThreadPool::schedule(const Task & task)
{
	lock_guard<mutex> ll(m_monitor);
	
	m_Tasks.push(task);
	m_taskCIN.notify_one();
}

void ThreadPool::wait() const
{
	unique_lock<mutex> lock(m_monitor);
	
	while(m_actThreadNum != 0 || m_Tasks.size() != 0)
		m_tasksFinishedOrTerminated.wait(lock);
}

void ThreadPool::timedwait(struct timeval & spec) const
{
}

void ThreadPool::timedwait(chrono::steady_clock::time_point abs_time) const
{
	unique_lock<mutex> lock(this -> m_monitor);

	while(m_actThreadNum != 0 || m_Tasks.size() != 0)
		m_tasksFinishedOrTerminated.wait_until(lock, abs_time);
}

bool ThreadPool::resize(int targetNum)
{
    unique_lock<mutex> lock(m_monitor);

    if(targetNum < m_actThreadNum)
      return false;
    else if(targetNum == m_threadNum)
      return true;
    else if(targetNum > m_threadNum)
    {
      while(m_threadNum < targetNum)
      {
        WorkThread::createThread(shared_from_this());
        m_threadNum++;
      }
    }
    else
    {
      /**
          Without implementation
      **/
        m_targetNum = targetNum;
       	while(m_threadNum == targetNum)
   			m_tasksFinishedOrTerminated.wait(lock);	
   		m_targetNum = 0;
    }
    return true;
}

void ThreadPool::workerDestructed(std::shared_ptr<WorkThread> worker)
{
	lock_guard<mutex> lock(m_monitor);
	
	m_actThreadNum--;  m_threadNum--;
	
	m_terminatedWorks.push_back(worker);
}

bool ThreadPool::executeTask()
{
	Task task;

	{ 
	  unique_lock<mutex> lock(m_monitor);

	  while(m_Tasks.size() == 0)
	  {
	  	if(m_targetNum != 0 && m_targetNum < m_threadNum)
	  	{	
	  		m_targetNum-- ; m_threadNum-- ;
	  		m_tasksFinishedOrTerminated.notify_one();
	  		
	  		return false;
	  	}
	  	else
	  	{
	  		m_taskCIN.wait(lock);
	  	}
	  }
	  m_actThreadNum++;
	  task = m_Tasks.front();
	  m_Tasks.pop();
	}

	if(task)
	{
	  task();

	  unique_lock<mutex> lock(m_monitor);
	  
	  m_tasksFinishedOrTerminated.notify_one();

	  m_actThreadNum--;
	}
	return true;
}

void ThreadPool::killAllThreads()
{
	unique_lock<mutex> lock(m_monitor);
	
	m_targetNum = 0;
	m_tasksFinishedOrTerminated.wait(lock);
}

void WorkThread::createThread(SPool const & pool)
{
	SWork worker(new WorkThread(pool));
	
	if(worker)
	{
		worker -> m_thread.reset(new thread(bind(&WorkThread::run, worker)));
	}
}

void RWLock::readLock()
{
	unique_lock<mutex> lock(m_monitor);
	if(m_nWriter || m_wWriter)
	{
		m_wReader++;
		while(m_nWriter || m_wWriter)
			m_condRead.wait(lock);
		m_wReader--;
	}
	m_nReader++;
}

void RWLock::readUnlock()
{
	unique_lock<mutex> lock(m_monitor);
	m_nReader--;
}

void RWLock::writeLock()
{
	unique_lock<mutex> lock(m_monitor);
	if(m_nReader || m_nWriter)
	{
		m_wWriter++;
		while(m_nReader || m_nWriter)
			m_condWrite.wait(lock);
		m_wWriter--;
	}
	m_wWriter++;
}

void RWLock::writeUnlock()
{
	unique_lock<mutex> lock(m_monitor);
	m_wWriter--;
}
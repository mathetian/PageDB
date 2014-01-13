#include "ThreadPool.h"

ThreadPool::ThreadPool(int threadNum)
{
	m_actThreadNum = 0;
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

bool ThreadPool::wait() const
{
	unique_lock<mutex> lock(m_monitor);
	
	while(m_actThreadNum != 0 || m_Tasks.size() != 0)
		m_tasksFinished.wait(lock);
}


bool ThreadPool::wait(struct timeval & spec) const
{
	unique_lock<mutex> lock(m_monitor);

	while(m_actThreadNum != 0 || m_Tasks.size() != 0)
		m_tasksFinished.wait(lock);
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
	    m_taskCIN.wait(lock);
	  }

	  task = m_Tasks.front();
	  m_Tasks.pop();
	}

	if(task)
	{
	  task();
	}
	return true;
}

void ThreadPool::killAllThreads()
{

}

void WorkThread::createThread(SPool const & pool)
{
	SWork worker(new WorkThread(pool));
	
	if(worker)
	{
		worker->m_thread.reset(new thread(bind(&WorkThread::run, worker)));
	}
}

int main()
{
	
}
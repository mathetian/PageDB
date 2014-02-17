#include "Thread.h"
using namespace utils;

#ifdef __WIN32
	#include "ThreadWin.cpp"
#else
	#include "ThreadPosix.cpp"
#endif

/**
	Two ways for Singleton
	
	Another way, not thread_safe
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

SingletonMutex& SingletonMutex::getInstance()
{
    static SingletonMutex instance;
    return instance;
}

ScopeMutex::ScopeMutex(Mutex * pmutex) :\
	 m_pmutex(pmutex) 
{ 
	m_pmutex -> lock(); 
}
  
ScopeMutex::~ScopeMutex() 
{ 
	m_pmutex -> unlock(); 
}

RWLock::RWLock(Mutex * mutex) : m_mutex(mutex), m_condRead(m_mutex), 
	m_condWrite(m_mutex), m_nReader(0), m_nWriter(0), m_wReader(0), m_wWriter(0) 
{ 
	if(mutex == NULL) m_mutex = new Mutex; 
}


void RWLock::readLock()
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

void RWLock::readUnlock()
{
	ScopeMutex scope(m_mutex);
	m_nReader--;

	if(m_wWriter != 0)
		m_condWrite.signal();
	else if(m_wReader != 0)
		m_condRead.signal();
}

void RWLock::writeLock()
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

void RWLock::writeUnlock()
{
	ScopeMutex scope(m_mutex);
	m_nWriter--;

	if(m_wWriter != 0)
		m_condWrite.signal();
	else if(m_wReader != 0)
		m_condRead.signal();
}
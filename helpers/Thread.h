#ifndef _THREAD_WIN_POS_H
#define _THREAD_WIN_POS_H

#ifdef __WIN32
	#include <windows.h>
	typedef HANDLE THRID;
	typedef unsigned(WINAPI *Task)(void *);
#else
	#include <pthread.h>
	typedef pthread_t THRID;
	typedef void *(*Task)(void *);
#endif

class Noncopyable
{
protected:
	Noncopyable() { }
	~Noncopyable() { }
private:
	Noncopyable( const Noncopyable& ) { }
	Noncopyable& operator=( const Noncopyable& ) { }
};

#ifdef __WIN32
	class Thread{
	public:
		Thread(Task task = NULL, void * args = NULL) : m_task(task), m_args(args) { }
		
		Thread(const Thread & thr) : m_task(thr.m_task), m_args(thr.m_args) { }

		THRID run() 
		{ 
			if(m_tid != NULL)  return m_tid; 
			
			m_tid = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)m_task, m_args, 0, NULL);

			return m_tid; 
		}

		void   join() { if(m_tid != NULL) WaitForSingleObject(m_tid, INFINITE); }
		
		void   cancel() { if(m_tid != NULL) TerminateThread(m_tid, 0); }
	
	private:
		THRID  m_tid;
		Task   m_task;
		void * m_args;
	};

	class CondVar;

	class Mutex{
	public:
		Mutex() { InitializeCriticalSection(&m_mutex); }
		virtual ~Mutex() { }
	public:
		void lock()   { EnterCriticalSection(&m_mutex);}
		void unlock() { LeaveCriticalSection(&m_mutex); }
	private: 
		CRITICAL_SECTION m_mutex;
		friend class CondVar;
	};

	class CondVar {
	public:
		CondVar(Mutex* mutex = NULL)
		{ 
			m_cond = CreateEvent(NULL, TRUE, FALSE, TEXT("m_cond"));
		}
		
		~CondVar() {  }
	public:
		void Wait() 
		{  
			WaitForSingleObject(m_cond, INFINITE);
		}

		void Signal() 
		{ 
			SetEvent(m_cond);
		}
		
		void SignallAll()
		{ 
			/**Sorry, didn't find enough materials to support it**/
		}
	private:
		HANDLE m_cond;
	};
#else
	class Thread{
	public:
		Thread(Task task = NULL, void * args = NULL) : m_task(task), m_args(args), m_tid(-1) { }
		
		Thread(const Thread & thr) : m_task(thr.m_task), m_args(thr.m_args) { }

		pthread_t run() 
		{ 
			if(m_tid != -1) 
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

	class CondVar;

	class Mutex{
	public:
		Mutex() { pthread_mutex_init(&m_mutex, 0); }
		virtual ~Mutex() { pthread_mutex_destroy(&m_mutex); }
	public:
		int lock() { return pthread_mutex_lock(&m_mutex);}
		int unlock() { return pthread_mutex_unlock(&m_mutex); }
		int trylock() { return pthread_mutex_trylock(&m_mutex); }
	private: 
		pthread_mutex_t m_mutex;
		friend class CondVar;
	};

	class CondVar {
	public:
		CondVar(Mutex* mutex) : m_mutex(mutex)  
		{ 
			pthread_cond_init(&m_cond, NULL); 
		}
		
		~CondVar() { pthread_cond_destroy(&m_cond); }
	public:
		void wait() 
		{  
			pthread_cond_wait(&m_cond, &m_mutex->m_mutex); 
		}

		void signal() 
		{ 
			pthread_cond_signal(&m_cond); 
		}
		
		void signallAll()
		{ 
			pthread_cond_broadcast(&m_cond); 
		}
	private:
		  pthread_cond_t m_cond;
		  Mutex* m_mutex;
	};
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

class SingletonMutex : Noncopyable
{
public:
	static SingletonMutex& getInstance()
	{
        static SingletonMutex instance;
        return instance;
    }
    ~SingletonMutex() { }
private:
    SingletonMutex() {}
public:
	Mutex m;
};

class ScopeMutex : Noncopyable{
public:
    ScopeMutex(Mutex * pmutex) : m_pmutex(pmutex) 
    { 
    	m_pmutex -> lock(); 
    }
    
    ~ScopeMutex() { m_pmutex -> unlock(); }
private: 
    Mutex * m_pmutex;
};

/**constructor can't be empty parameter.**/
class RWLock{
public:
    RWLock(Mutex * mutex = NULL) : m_mutex(mutex), m_condRead(m_mutex), m_condWrite(m_mutex),\
    	 m_nReader(0), m_nWriter(0), m_wReader(0), m_wWriter(0) { if(mutex == NULL) m_mutex = new Mutex; }
   ~RWLock() { }

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

public:
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

public:
    bool specilLock()
    {
    	ScopeMutex scope(m_mutex);

    	if(m_nReader == 1 && m_nWriter == 0)
    	{
    		m_nReader--;
    		m_nWriter++;
    		return true;
    	}
    	else return false;
    }

    void specilUnlock()
    {

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

/**Todo list**/
class ReentrantLock{

};

class Atomic
{
public:
	Atomic(int r) : obj(r) { }

	operator int()
	{
        return obj;
    }

    Atomic & operator=( int val )
    {
        obj = val;
        __sync_synchronize();
        return *this;
    }

    int operator++()
    {
        return __sync_add_and_fetch( &obj, 1 );
    }

    int operator++( int )
    {
        return __sync_fetch_and_add( &obj, 1 );
    }

    int operator+=( int val )
    {
        return __sync_add_and_fetch( &obj, val );
    }

    int operator--()
    {
        return __sync_sub_and_fetch( &obj, 1 );
    }

    int operator--( int )
    {
        return __sync_fetch_and_sub( &obj, 1 );
    }

    int operator-=( int val )
    {
        return __sync_sub_and_fetch( &obj, val );
    }

    int addAndGet( int val)
    {
    	return __sync_add_and_fetch(&obj, val);
    }

    // Perform an atomic CAS operation
    // returning the value before the operation
    int exchange( int oldval, int newval )
    {
        return __sync_val_compare_and_swap( &obj, oldval, newval );
    }
private: int obj;
};

#endif
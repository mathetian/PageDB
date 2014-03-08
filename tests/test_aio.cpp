#include "../utils/AIO.h"
#include "../utils/Thread.h"
using namespace utils;

#include <iostream>
#include <string>
using namespace std;
#include <stdlib.h> 
#include <string.h>


#define STRLEN 1250
#define STRCNT 1800

string getrandstr()
{
	string str;
	int len = STRLEN;
	for(int i=0;i<len;i++)
		str.push_back(rand()%26+'a');
	return str;
}

int count = 0;
Mutex m_lock;
CondVar m_cond(&m_lock);

void call(int id)
{
	m_lock.lock();
	count++;
	int oc = count;
	m_lock.unlock();

	if(oc == STRCNT)
	{
		m_cond.signal();
	} 
	cout<<oc<<endl;
}

void RunTest1()
{
	AIOFile file;
	file.AIO_Open("hello3.txt");

	for(int i=0;i<STRCNT;i++)
	{
		string str = getrandstr();
		file.AIO_Write((char*)str.c_str(), -1, str.size(), i, call);
	}

	{
		m_lock.lock();
		while(count != STRCNT) m_cond.wait();
		m_lock.unlock();
		file.AIO_Close();
	}

	cout<<"Over"<<endl;
}

void RunTest2()
{
	AIOFile file;
	file.AIO_Open("hello3.txt");

	file.AIO_Close();
	cout<<"Over"<<endl;
}

void RunTest3()
{
	AIOFile file;
	file.AIO_Open("hello3.txt");

	count = 0;
	char **str = new char*[STRCNT];

	for(int i=0;i<STRCNT;i++)
	{
		str[i] = new char[STRLEN+1];
		memset(str[i], 0, STRLEN+1);
		printf("???? %d\n", i);
		file.AIO_Read(str[i], i*STRLEN, STRLEN, i, call);
	}

	{
		m_lock.lock();
		while(count != STRCNT) m_cond.wait();
		m_lock.unlock();

		file.AIO_Close();
	}
	
	for(int i=0;i<STRCNT;i++)
	{
		cout<<str[i]<<endl;
	}	
}

void RunTest4()
{
	AIOFile file;
	file.AIO_Open("hello3.txt");
	count = 0;
	for(int i=0;i<STRCNT;i++)
	{
		string str = getrandstr();
		file.AIO_Write((char*)str.c_str(), i*STRLEN, str.size(), i, call);
	}

	{
		m_lock.lock();
		while(count != STRCNT) m_cond.wait();
		m_lock.unlock();
		file.AIO_Close();
	}
	
	cout<<"Over"<<endl;
}

#include "../include/BufferPacket.h"
using namespace customdb;

void RunTest5()
{
	AIOFile file;
	file.AIO_Open("hello4.txt");
	count = 0;
	for(int i=0;i<STRCNT;i++)
	{
		cout<<"count:"<<i<<endl;
		BufferPacket packet(STRLEN);
		file.IO_Write(packet.getData(), i*STRLEN, STRLEN);	
	}
	
	cout<<"Over"<<endl;
}

int main()
{	
	// RunTest1();
	// printf("Test1 finished\n");
	// RunTest2();
	// printf("Test2 finished\n");
	// RunTest3();
	// printf("Test3 finished\n");
	RunTest5();
	return 0;
}


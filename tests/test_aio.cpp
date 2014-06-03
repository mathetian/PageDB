#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "AIO.h"
#include "Thread.h"
#include "TestUtils.h"
using namespace utils;

#include "BufferPacket.h"
using namespace customdb;

#include <stdlib.h>
#include <string.h>

#define STRLEN 1250
#define STRCNT 1800

class A { };

string getrandstr()
{
    string str;
    int len = STRLEN;
    for(int i=0; i<len; i++)
        str.push_back(rand()%26+'a');
    return str;
}

int count = 0;
Mutex m_lock;
CondVar m_cond(&m_lock);

void call(int id)
{
    int oc;

    {
        ScopeMutex scope(&m_lock);
        count++;
        oc = count;
    }

    if(oc == STRCNT)
        m_cond.signal();

}

TEST(A, Test1)
{
    vector<string> rss;
    {

        AIOFile file;
        file.AIO_Open("hello3.txt");

        for(int i=0; i<STRCNT; i++)
        {
            string str = getrandstr();
            rss.push_back(str);
            file.AIO_Write((char*)str.c_str(), -1, str.size(), i, call);
        }

        {
            m_lock.lock();
            while(count != STRCNT) m_cond.wait();
            m_lock.unlock();
            file.AIO_Close();
        }

        cout<<"Step 1 Over"<<endl;

    }


    /**
    *** Step 2
    ***/
    {
        AIOFile file;
        file.AIO_Open("hello3.txt");

        count = 0;
        char **str = new char*[STRCNT];

        for(int i=0; i<STRCNT; i++)
        {
            str[i] = new char[STRLEN+1];
            memset(str[i], 0, STRLEN+1);
            file.AIO_Read(str[i], i*STRLEN, STRLEN, i, call);
        }

        {
            m_lock.lock();
            while(count != STRCNT) m_cond.wait();
            m_lock.unlock();

            file.AIO_Close();
        }

        for(int i=0; i<STRCNT; i++)
        {
            ASSERT_EQ(rss.at(i), str[i]);
        }
        //cout<<str[i]<<endl;
        cout<<"Step 2 Over"<<endl;
    }

}

int main()
{
    RunAllTests();
    return 0;
}


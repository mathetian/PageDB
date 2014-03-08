#ifndef _AIO_H
#define _AIO_H

#include <aio.h>
#include <stdio.h>
#include <fcntl.h>
#include <libaio.h>
#include <unistd.h>
#include <strings.h>
#include <stdint.h>

#include <assert.h>
#include <errno.h> 
#include <iostream>
using namespace std;

#include "../utils/Thread.h"

#define DefaultMode S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

namespace utils{

typedef void (*Callback) (int);

class IORequest{
public:
    IORequest(int size, Callback callback = NULL) : \
        m_size(size), callback(callback) { }

    virtual void PostExecute(int status) = 0;
protected:
    int m_size;
    Callback callback;
};

class AIOFile;

class BIORequest : IORequest{
public:
    BIORequest(int size) : \
        m_cond(&m_mutex), IORequest(size), m_wake(0) { }

    void PostExecute(int status)
    {
        m_wake = 1;
        while(m_wake == 1) m_cond.signal();
    }

    virtual ~BIORequest()
    {
        if(m_wake == 1)
            m_cond.signal();
    }
    
private:
    Mutex   m_mutex;
    CondVar m_cond;
    int    m_wake;
    friend class AIOFile;
};

struct ThreadArg2
{
    BIORequest *bm;
    void (BIORequest::*method)(int);
};

class AIORequest : IORequest{
public:
    AIORequest(int size, int index = -1, Callback callback = NULL) : \
                m_index(index), IORequest(size, callback), m_data(NULL) { }

public:
    void PostExecute(int status)
    {
        if(status != m_size)
            cout<<status<<" "<<m_size<<endl;
        assert(status == m_size);
        if(m_data == NULL)
        {
            if(callback)
                callback(m_index);
        }   
        else
        {
            ThreadArg2 *parg = (ThreadArg2*)m_data;
            (parg->bm->*(parg->method))(status);
        }
    }

private:
    int    m_index;
    void  *m_data;
    friend class AIOFile;
};

class AIOFile{
public:
    AIOFile() : fd(-1), thr(NULL), ioctx(0), m_stop(false) { }
   
   ~AIOFile() 
   { 
        if(fd>=0)
        AIO_Close();
   }

public:
    struct ThreadArg
    {
        AIOFile* bm;
        void (AIOFile::*method)(void);
    };

    static void* ThreadBody(void *arg)
    {
        ThreadArg *thrargs = (ThreadArg*)arg;
        (thrargs->bm->*(thrargs->method))();
    }

public:
    void AIO_Open(const char * fileName, int oflag = O_RDWR, mode_t mode = 0644)
    {
        fd = open(fileName, oflag | O_CREAT, 0644);
        assert((fd != -1) && ("AIO_Open error\n"));
        assert(io_setup(100, &ioctx) == 0);

        sarg.bm = this;
        sarg.method = &AIOFile::run;

        thr = new Thread(&AIOFile::ThreadBody, &sarg);
        thr -> run();
        fileLen = lseek(fd, 0, SEEK_END);
        
        assert(fileLen != -1);        
    }

    void AIO_Close()
    {
        ScopeMutex scope(&m_mutex);  
        m_stop = true;      
        thr -> join();
        fsync(fd);
        close(fd);
    }

    void AIO_Read(char * buf, int offset, int size, int index = -1, Callback callback = NULL)
    {
        if(offset == -1)
        {
            offset = File_Len();
            fileLen += size;   
        }  
        else if(offset + size > fileLen)
            fileLen = offset + size;

        struct iocb  iocb;
        struct iocb* iocbs = &iocb;
        
        AIORequest *req = new AIORequest(size, index, callback);
        io_prep_pread(&iocb, fd, buf, size, offset);
        iocb.data = req;

        if(io_submit(ioctx, 1, &iocbs) != 1)
        {
            printf("Error: %s\n",strerror(errno));
            assert(0);
        }
    }

    void AIO_Write(char * buf, int offset, int size, int index = -1, Callback callback = NULL)
    {
        if(offset == -1) 
        {
            offset   = File_Len();
            fileLen += size;
        }
        else if(offset + size > fileLen)
            fileLen = offset + size;

        struct iocb  iocb;
        struct iocb* iocbs = &iocb;
        AIORequest *req = new AIORequest(size, index, callback);
        io_prep_pwrite(&iocb, fd, buf, size, offset);
        iocb.data = req;

        if(io_submit(ioctx, 1, &iocbs) != 1)
        {
            printf("Error: %s\n",strerror(errno));
            assert(0);
        }
    }

private:
    void AIO_Read2(char * buf, int offset, int size, void *data)
    {
        struct iocb  iocb;
        struct iocb* iocbs = &iocb;
        
        AIORequest *req = new AIORequest(size);
        req -> m_data = data;
        io_prep_pread(&iocb, fd, buf, size, offset);
        iocb.data = req;

        if(io_submit(ioctx, 1, &iocbs) != 1)
        {
            printf("Error: %s\n",strerror(errno));
            assert(0);
        }
    }

    void AIO_Write2(char * buf, int offset, int size, void *data)
    {
        struct iocb  iocb;
        struct iocb* iocbs = &iocb;
        
        AIORequest *req = new AIORequest(size);
        req -> m_data = data;
        io_prep_pwrite(&iocb, fd, buf, size, offset);
        iocb.data = req;

        if(io_submit(ioctx, 1, &iocbs) != 1)
        {
            printf("Error: %s\n",strerror(errno));
            assert(0);
        }
    }

public:
    uint32_t IO_Read(char * buf, int offset, int size)
    {
        if(offset == -1)
        {
            offset = File_Len();
            fileLen += size;   
        }
        else if(offset + size > fileLen)
            fileLen = offset + size;

        BIORequest* request = new BIORequest(size);

        request->m_mutex.lock();
        ThreadArg2 arg2;
        arg2.bm     = request;
        arg2.method = &BIORequest::PostExecute;
        AIO_Read2(buf, offset, size, &arg2);        
        
        while(request->m_wake == 0) 
            request->m_cond.wait();
        request->m_wake = 2;
        request->m_mutex.unlock();
        delete request; request = NULL;

        return offset;
    }

    uint32_t IO_Write(char * buf, int offset, int size)
    {
        if(offset == -1) 
        {
            offset = File_Len();
            fileLen += size;
        }
        else if(offset + size > fileLen)
            fileLen = offset + size;

        BIORequest* request = new BIORequest(size);

        request->m_mutex.lock();
        ThreadArg2 arg2;
        arg2.bm     = request;
        arg2.method = &BIORequest::PostExecute;
        AIO_Write2(buf, offset, size, &arg2);

        while(request->m_wake==0)
            request->m_cond.wait();
        request -> m_wake = 2;
        request->m_mutex.unlock();

        delete request; request = NULL;   

        return offset;  
    }   

    uint32_t File_Len()
    {
        return fileLen;
    }

private:
    void run()
    {
        struct io_event* events = new io_event[100];

        while(m_stop == false)
        {
            struct timespec timeout;
            timeout.tv_sec = 0;
            timeout.tv_nsec = 100000000;

            int num_events;
            num_events = io_getevents(ioctx, 1, 100, events, &timeout);

            if (num_events < 0) 
            {
                if (-num_events != EINTR) 
                {
                    cout << "io_getevents error: " << strerror(-num_events);
                    break;
                } else
                    continue;
            }

            for (int i = 0; i < num_events; i++) 
            {
              struct io_event event = events[i];
              AIORequest* req = static_cast<AIORequest*>(event.data);
              req -> PostExecute(event.res);
              
              delete req;
            }
        }

        delete [] events;
        events = NULL;
    }

private:
    int fd;
    io_context_t ioctx;
    Mutex m_mutex;
    Thread * thr; 
    uint32_t fileLen;
    ThreadArg  sarg;
    bool   m_stop;

};

}

#endif
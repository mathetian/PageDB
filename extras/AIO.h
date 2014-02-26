#ifndef _AIO_H
#define _AIO_H

#include <thread>
#include <mutex>
#include <functional>
using namespace std;

#include <aio.h>

#include <stdio.h>
#include <fcntl.h>
#include <libaio.h>
#include <unistd.h>
#include <strings.h>

#include <assert.h>

#define DefaultMode S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

#define DEFINE_string(a,b) string FLAGS_##a = b
#define DEFINE_int32(a,b) int FLAGS_##a = b

DEFINE_int32(file_size, 1000);
DEFINE_int32(concurrent_requests, 100);
DEFINE_int32(min_nr, 1);
DEFINE_int32(max_nr, 1);

typedef function<void(int) > Callback;

class AIORequest{
public:
    AIORequest(const char * buf, int offset, int size, Callback callback) : \
                    buf(buf), offset(offset), size(size), callback(callback){ }
   ~AIORequest() { }

public:
    virtual void PostExecute(int status) = 0;

protected:
    const char * buf;
    int offset;
    int size;
    Callback callback;
};

class AIOReadRequest : public AIORequest{
public:
    AIOReadRequest(const char * buf, int offset, int size, Callback callback) : AIORequest(buf,offset,size,callback){ }
   ~AIOReadRequest() { }

public:
    void PostExecute(int status)
    {
        if(status != size) 
            printf("Read Error, %d\n",status);
        if(callback)
            callback(status == size ? 1 : 0);
    }
};

class AIOWriteRequest : public AIORequest{
public:
    AIOWriteRequest(const char * buf, int offset, int size, Callback callback) : AIORequest(buf,offset,size,callback){ }
   ~AIOWriteRequest() { }

private:
    void PostExecute(int status)
    {
        if(status != size) 
            printf("Write Error, %d\n",status);
        if(callback)
            callback(status == size ? 1 : 0);
    }
};

class AIOFile{
public:
    AIOFile() { fd = -1; thr = NULL;}
   ~AIOFile() { if(thr) thr -> join(); if(thr) delete thr; thr = NULL; if(fd != -1) close(fd);}

public:
    void AIO_Open(const char * fileName, int oflag = O_WRONLY, mode_t mode = DefaultMode)
    {
        fd = open(fileName, oflag | O_DIRECT | O_CREAT, mode);
        assert((fd != -1) && ("AIO_Open error\n"));
        thr = new thread(bind(&AIOFile::run, this));
        io_setup(100, &ioctx);
    }

    void AIO_Close()
    {
        m_mutex.lock();
        
        assert(fd != -1);
        thr -> join(); thr = NULL; 
        close(fd);
        
        m_mutex.unlock();
    }

    void AIO_Read(char * buf, int offset, int size, Callback callback = NULL)
    {
        struct iocb  iocb;
        struct iocb* iocbs = &iocb;
        
        AIORequest *req = new AIOReadRequest(buf, offset, size, callback);
        io_prep_pread(&iocb, fd, buf, size, offset);
        iocb.data = req;
        int res = io_submit(ioctx, 1, &iocbs);
        assert(res == 1);
    }

    void AIO_Write(char * buf, int offset, int size, Callback callback = NULL)
    {
        struct iocb  iocb;
        struct iocb* iocbs = &iocb;
        
        AIORequest *req = new AIOWriteRequest(buf, offset, size, callback);
        io_prep_pread(&iocb, fd, buf, size, offset);
        iocb.data = req;
        int res = io_submit(ioctx, 1, &iocbs);
        assert(res == 1);
    }


private:
    void run()
    {
        struct io_event* events = new io_event[FLAGS_max_nr];

        while(m_mutex.try_lock())
        {
            
            struct timespec timeout;
            timeout.tv_sec = 10;
            timeout.tv_nsec = 0;
            
            int num_events;
            num_events = io_getevents(ioctx, FLAGS_min_nr, FLAGS_max_nr, events, &timeout);
            
            for (int i = 0; i < num_events; i++) 
            {
              struct io_event event = events[i];
              AIORequest* req = static_cast<AIORequest*>(event.data);
              req -> PostExecute(event.res);
              delete req;
            }
            m_mutex.unlock();
        }
        delete [] events;
        events = NULL;
    }

private:
    int fd;
    io_context_t ioctx;
    mutable mutex m_mutex;
    mutable thread * thr; 
};
#endif
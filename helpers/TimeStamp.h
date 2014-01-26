#ifndef _TIME_STAMP_H
#define _TIME_STAMP_H

#include <sys/time.h>
#include <time.h>
#include <stdio.h>

class TimeStamp
{
public:
    static void StartTime()
    {
        gettimeofday(&starttime, NULL);
    }

    static void StopTime()
    {
        gettimeofday(&curtime, NULL);
    }

    static void StopTime(const char * str)
    {
        StopTime();
        ElapseTime(str);
    }

    static void ElapseTime(const char * str)
    {
        timeval_subtract(&difference,&curtime,&starttime);
        printf("%s %lld s, %lld us\n", str, (long long)difference.tv_sec, (long long)difference.tv_usec);
    }

    static void AddTime(struct timeval & totalTime)
    {
        totalTime.tv_sec  += difference.tv_sec;
        totalTime.tv_usec += difference.tv_usec;
        if(totalTime.tv_usec >= 1000000)
        {
            totalTime.tv_sec++;
            totalTime.tv_usec -= 1000000;
        }
    }

    static double GetCurTimeAsDouble()
    {
        struct timeval cur;
        gettimeofday(&cur,NULL);
        double rs = 0;
        rs = cur.tv_sec;
        rs += (cur.tv_usec/1000000.0);
        
        return rs;        
    }

private:
    static int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
    {
        if (x->tv_usec < y->tv_usec)
        {
            int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
            y->tv_usec -= 1000000 * nsec;
            y->tv_sec += nsec;
        }
        if (x->tv_usec - y->tv_usec > 1000000)
        {
            int nsec = (x->tv_usec - y->tv_usec) / 1000000;
            y->tv_usec += 1000000 * nsec;
            y->tv_sec -= nsec;
        }

        result->tv_sec = x->tv_sec - y->tv_sec;
        result->tv_usec = x->tv_usec - y->tv_usec;

        return x->tv_sec < y->tv_sec;
    }

private:
    static struct timeval starttime,curtime,difference;
};

struct timeval TimeStamp::starttime;
struct timeval TimeStamp::curtime;
struct timeval TimeStamp::difference;

#endif
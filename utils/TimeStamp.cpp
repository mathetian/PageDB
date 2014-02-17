#include "TimeStamp.h"

using namespace utils;

void TimeStamp::StartTime()
{
#ifdef _WIN32
    starttime = DW2time(GetTickCount());
#else
    gettimeofday(&starttime, NULL);
#endif
}

void TimeStamp::StopTime()
{
#ifdef _WIN32
    curtime = DW2time(GetTickCount());
#else
    gettimeofday(&curtime, NULL);
#endif
}

void TimeStamp::StopTime(const char * str)
{
    StopTime();
    ElapseTime(str);
}

void TimeStamp::ElapseTime(const char * str)
{
    SubtractTime(&difference,&curtime,&starttime);
    cout<<str<<" "<<difference.tv_sec<<"s "<<difference.tv_usec<<"us"<<endl;
}

struct timeval TimeStamp::GetDiffTime()
{
    SubtractTime(&difference, &starttime, &curtime);
    return difference;
}

int TimeStamp::SubtractTime(struct timeval *result, struct timeval *x, struct timeval *y)
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

struct timeval TimeStamp::DW2time(int dur)
{
    struct timeval rs;
    rs.tv_sec  = dur / 1000;
    rs.tv_usec = (dur % 1000) * 1000;
    return rs;
}

TimeAccumulator::TimeAccumulator()
{
    ResetTime();
}

void TimeAccumulator::ResetTime()
{
    m_totalTime.tv_sec  = 0;
    m_totalTime.tv_usec = 0;
}

void TimeAccumulator::StartTime()
{
    m_ts.StartTime();
}

void TimeAccumulator::StopTime()
{
    m_ts.StopTime();
    m_difference = m_ts.GetDiffTime();

    m_totalTime.tv_sec  += m_difference.tv_sec;
    m_totalTime.tv_usec += m_difference.tv_usec;

    if(m_totalTime.tv_usec >= 1000000)
    {
        m_totalTime.tv_sec++;
        m_totalTime.tv_usec -= 1000000;
    }
}

void TimeAccumulator::PrintTime(const char *str)
{
    cout<<str<<" :"<<m_totalTime.tv_sec<<"s "<<m_totalTime.tv_usec<<"us"<<endl;

    ResetTime();
}



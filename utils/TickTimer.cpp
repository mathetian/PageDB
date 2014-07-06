// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "TickTimer.h"
using namespace utils;

void Timer::Start()
{
#ifdef _WIN32
    starttime = DW2time(GetTickCount());
#else
    gettimeofday(&starttime, NULL);
#endif
}

void Timer::Stop()
{
#ifdef _WIN32
    curtime = DW2time(GetTickCount());
#else
    gettimeofday(&curtime, NULL);
#endif
}

void Timer::Print(const char * str)
{
    SubtractTime(&difference, &curtime, &starttime);
    m_os<< str << " " << difference.tv_sec << "s " << difference.tv_usec << "us" <<endl;
}

struct timeval Timer::GetDiffTime()
{
    SubtractTime(&difference, &curtime, &starttime);
    return difference;
}

int Timer::SubtractTime(struct timeval *result, struct timeval *x, struct timeval *y)
{
    if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }

    if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (x->tv_usec - y->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    return x->tv_sec < y->tv_sec;
}

struct timeval Timer::DW2time(int dur)
{
    struct timeval rs;
    rs.tv_sec  = dur / 1000;
    rs.tv_usec = (dur % 1000) * 1000;
    return rs;
}

TimeAccumulator::TimeAccumulator(ostream &os) : m_os(os)
{
    ResetTimer();
}

void TimeAccumulator::ResetTimer()
{
    m_totalTime.tv_sec  = 0;
    m_totalTime.tv_usec = 0;
}

void TimeAccumulator::StartTimer()
{
    m_ts.Start();
}

void TimeAccumulator::StopTimer()
{
    m_ts.Stop();
    struct timeval m_difference = m_ts.GetDiffTime();

    m_totalTime.tv_sec  += m_difference.tv_sec;
    m_totalTime.tv_usec += m_difference.tv_usec;

    if(m_totalTime.tv_usec >= 1000000) {
        m_totalTime.tv_sec++;
        m_totalTime.tv_usec -= 1000000;
    }
}

void TimeAccumulator::Print(const char *str)
{
    m_os<<str<< " " << m_totalTime.tv_sec << "s " << m_totalTime.tv_usec << "us" <<endl;
}

// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _TICK_TIMER_H
#define _TICK_TIMER_H

#include "CommonHeader.h"

/**
** TickTimer provides `Timer` and `TimerAccumlator`
***/
namespace utils
{
/**
** Timer is used to get the elapsed time since special time.
**/
class Timer
{
public:
    Timer(ostream &os = cout) : m_os(os) { }

public:
    /**
    ** Timer starts with StartTime and ends with StopTime
    ** Anyone can call PrintElapsedTime(const char*) to print the elapsed time.
    **/
    void Start();
    void Stop();
    void PrintElapsedTime(const char * str);
    /**
    ** Return Elapsed Time
    **/
    struct timeval GetDiffTime();

private:
    int SubtractTime(struct timeval *result, struct timeval *x, struct timeval *y);
    struct timeval DW2time(int dur);

private:
    struct timeval starttime, curtime, difference;
    ostream &m_os;
};

/**
** TimeAccumulator is used to compute the accumulated timer
**/
class TimeAccumulator
{
public:
    TimeAccumulator(ostream &os = cout);

public:
    void ResetTimer();
    void StartTimer();

    /**
    ** Each time when we call StopTime
    ** the timer will stop and calucate the total time
    **/
    void StopTimer();
    void PrintElapsedTime(const char * str = NULL);

private:
    Timer  m_ts;
    struct timeval m_totalTime;
    ostream &m_os;
};

}
#endif
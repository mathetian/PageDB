// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _TIME_STAMP_H
#define _TIME_STAMP_H

#include <iostream>
using namespace std;

#include <time.h>

#ifdef _WIN32
#pragma warning(disable : 4786)
#pragma warning(disable : 4996)
#include <Windows.h>
#else
#include <sys/time.h>
#endif

namespace utils
{

class TimeStamp
{
public:
    void StartTime();
    void StopTime();
    void StopTime(const char * str);
    void ElapseTime(const char * str);
    struct timeval GetDiffTime();

private:
    int SubtractTime(struct timeval *result, struct timeval *x, struct timeval *y);
    struct timeval DW2time(int dur);

private:
    struct timeval starttime, curtime, difference;
};

class TimeAccumulator
{
public:
    TimeAccumulator();
public:
    void ResetTime();
    void StartTime();
    void StopTime();
    void PrintTime(const char * str = NULL);
private:
    TimeStamp m_ts;
    struct timeval m_totalTime;
    struct timeval m_difference;
};

}
#endif
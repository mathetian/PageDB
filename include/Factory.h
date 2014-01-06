#ifndef _FAC_H
#define _FAC_H

#include <string>
#include <iostream>
using namespace std;

#include "Log.h"

extern int defaultHashFunc(const string&str);

class Factory
{
public:
    Factory() { log = Log::GetInstance(); }
    virtual ~ Factory() {}
public:
    virtual bool   put(const string&key,const string&value) = 0;
    virtual string get(const string&key) = 0;
    virtual bool   remove(const string&key) = 0;
    virtual bool   init(const string&filename) = 0;
protected:
	int 	flag;
	Log  *  log;
};


#endif
#ifndef _FAC_H
#define _FAC_H

#include <string>
#include <fstream>
#include <iostream>
using namespace std;

#include "Log.h"
#include "Slice.h"

class Factory
{
public:
    Factory() { log = Log::GetInstance(); }
    virtual ~Factory() {}

public:
    virtual bool   put(const Slice & key,const string & value) = 0;
    virtual Slice  get(const Slice & key) = 0;
    virtual bool   remove(const Slice & key) = 0;
    virtual bool   init(const char * filename) = 0;

protected:
	Log  *  log;
};

/**Two samples to describe how to use it**/
inline int defaultHashFunc(const Slice & key)
{
    int index;
    int value = 0x238F13AF * key.size();
    for(index = 0; index < key.size(); index++)
        value = (value + (key[index] << (index*5 % 24))) & 0x7FFFFFFF;
    return value;
}

#endif
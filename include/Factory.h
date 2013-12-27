#ifndef _FAC_H
#define _FAC_H

#include <string>
using namespace std;

class Factory{
public:
	Factory() {}
	virtual ~ Factory() {}
public:
	virtual bool   put(const string&key,const string&value) = 0;
	virtual string get(const string&key) = 0;
	virtual bool   remove(const string&key) = 0;
	virtual bool   init() = 0;
};

#endif
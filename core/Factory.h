#ifndef _ENV_H
#define _ENV_H

#include <string>
using namespace std;

class Factory{
public:
	Factory() {}
	virtual ~ Factory() {}
public:
	virtual bool   put(const string&key,const string&value) = 0;
	virtual string get(const string&key);
	virtual bool   remove(const string&key);
};

#endif
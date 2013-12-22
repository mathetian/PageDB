#ifndef _BASE_CACHE_H
#define _BASE_CACHE_H

#include <string>
#include <vector>
using namespace std;

class BaseCache{
public:
	virtual bool 		   put(const string&key,const string&value)=0;
	virtual string  	   get(const string&key)=0;
	virtual void 		   remove(const string&key)=0;
	virtual vector<string> keys(vector<string>&rs)=0;
	virtual void           clear();
public:
	int errorState();
private:
	int lastflag;
};

#endif
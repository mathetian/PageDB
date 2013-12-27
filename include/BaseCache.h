#ifndef _BASE_CACHE_H
#define _BASE_CACHE_H

#include <string>
#include <vector>
#include <map>
using namespace std;

class BaseCache{
public:
	BaseCache();
	virtual ~ BaseCache();
public:
	virtual bool 		   put(const string&key,const string&value);
	virtual string  	   get(const string&key);
	virtual void 		   remove(const string&key);
	virtual vector<string> keys();
	virtual void           clear();
public:
	int errorState();
private:
	int lastflag;
	map<string, string&> softMap;
};

#endif
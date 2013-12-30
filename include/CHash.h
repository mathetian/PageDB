#ifndef _C_HASH_H
#define _C_HASH_H

#include <list>
#include <map>
#include <vector>
using namespace std;

#include "Factory.h"

typedef int (*HASH)(const string&key);

extern int defaultHashFunc(const string&str);

class ChainHash;

class Chain{
public:
	  Chain();
	~ Chain();
private:
	bool     put(const string&key,const string&value);
  	string   get(const string&key);
    bool     remove(const string&key);
private:
	map <string,string> ssMap;
	friend class ChainHash;
};

class ChainHash : public Factory{
public:
      ChainHash(int chainCount = 100, HASH hashFunc = defaultHashFunc);
    ~ ChainHash();
public:
  	bool     put(const string&key,const string&value);
  	string   get(const string&key);
    bool     remove(const string&key);
    bool	 init(const string&filename);
private:
	Chain  * getChain(const string&key);
private:
	int      chainCount;
	int		      curId;
	HASH       hashFunc;
	Chain  *      chain;
	vector <Chain*> header;
};

#endif
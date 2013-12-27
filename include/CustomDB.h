#ifndef _CUSTOM_DB_H
#define _CUSTOM_DB_H

#include "Log.h"
#include "Env.h"

#include "Option.h"
#include "Factory.h"
#include "BaseCache.h"

#include <string>
using namespace std;

class CustomDB{
public:
	CustomDB();
	virtual ~ CustomDB();
public:
	bool 	open(Options option);
	bool 	put(const string&key,const string&value);
	string  get(const string&key);
	/*bool	replace(const string&key,const string&value);*/
	bool 	remove(const string&key);
	int 	error();
private:
	Env1       * env1;
	int 	    errorStatus;
	Options 	option;
	Factory   * factory;
	BaseCache * cache;
	Log       * log;
	/*friend class Env;*/
};
#endif
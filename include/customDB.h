#ifndef _CUSTOM_DB_H
#define _CUSTOM_DB_H

#include "option.h"
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
	bool	replace(const string&key,const string&value);
	int 	error();
private:
	int 	errorStatus;
	Options 	option;
	Factory* factory;
	BaseCache* cache;
	Env*env;
};
#endif
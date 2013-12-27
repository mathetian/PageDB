#ifndef _CUSTOM_DB_H
#define _CUSTOM_DB_H

#include "Log.h"
#include "Option.h"
#include "Factory.h"
#include "BaseCache.h"

#include <string>
using namespace std;

#define ERROR 0
#define SUCCE 1

class CustomDB{
public:
	CustomDB();
	virtual ~ CustomDB();

public:
	bool 	open(const Options&option);
	bool 	put(const string&key,const string&value);
	string  get(const string&key);
	bool 	remove(const string&key);
	bool	getError();

private:
	Options   	option;
	Factory   * factory;
	BaseCache * cache;
	Log       * log;
	int         errorStatus;

public:
	bool init();

private:
	FILE	 * idxFile;
	FILE     * datFile;
};

#endif
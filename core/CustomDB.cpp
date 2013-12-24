#include "../include/customDB.h"
#include "Factory.h"

CustomDB::CustomDB()
{
}

CustomDB::~CustomDB()
{
	if(factory)
	{
		delete factory;
		factory = NULL;
	}
	
	if(env)
	{
		delete env;
		env=NULL;
	}

	if(cache)
	{
		delete cache;
		cache=NULL;
	}
}

bool CustomDB::open(Options&option)
{
	this->option = option;

	switch(option -> cacheOption -> cacheType)
	{
	case FIFO: 
		cache = new FIFOLimitedMemoryCache();
	break;
	default: 
		Log::e("CustomDB::open::cacheType error\n");
		break;
	}

	if(!cache)
		Log::e("CustomDB::open::new cache error\n");

	switch(option -> factoryOption -> factoryType)
	{
	case EHASH:
		factory = new ExtendibleHash();break;
	case CHASH:
		factory = new ChainHash();break;
	default:
		Log::e("CustomDB::open::factory error\n");
	}

	if(!factory)
		Log::e("CustomDB::open::new factory error\n");

	env = new Env(this);

	if(!env)
		Log::e("CustomDB::open::new Env error\n");

	if(env -> init() == 0)
		Log::e("CustomDB::open::env open error\n");
}

bool CustomDB::put(const string&key,const string&value)
{
	errorStatus = 0;
	if(cache->get(key))
		Log.w("CustomDB::put::exist in cache\n");
	else
	{
		if(!factory -> put(key,value))
			Log.w("CustomDB::put::factory put error\n");
		else 
			errorStatus = 1;
	}
	return errorStatus;
}

/*bool CustomDB::replace(const string&key,const string&value)
{
	errorStatus = 0;
	if(cache->get(key))
	{
		cache->update(key);
		errorStatus=1;
	}
	else
	{
		if(!factory->replace(key,value))
			Log.w("CustomDB::replace::factory replace error\n");
		else errorStatus=1;
	}
	return errorStatus;
}*/

string CustomDB::get(const string&key)
{
	errorStatus = 0;
	string rs = cache -> get(key);
	if(rs) errorStatus = 1;
	else
	{
		rs = factory -> get(key);
		if(!rs) 
			Log::w("CustomDB::get::factor get error\n");
		else errorStatus = 1;
	}
	return rs;
}

bool CustomDB::remove(const string&key)
{
	errorStatus = 1;
	string rs = cache -> get(key);
	if(rs)
	{
		int rflag = cache -> remove(key);
		if(!rflag)
			Log::e("CustomDB::remove::cache remove error\n");
	}
	int rflag = factory -> remove(key);
	if(!rflag)
		Log::e("CustomDB::remove::factory remove error\n");
	return errorStatus;
}

int CustomDB::error()
{
	return errorStatus;
}
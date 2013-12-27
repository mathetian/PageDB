#include "../include/customDB.h"


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
		env = NULL;
	}

	if(cache)
	{
		delete cache;
		cache=NULL;
	}
}

bool CustomDB::open(Options&option)
{
	this -> option = option;

	log = Log::GetInstance();
	log -> SetLogInfo(option -> logLevel, option -> prefix);

	switch(option -> cacheOption -> cacheType)
	{
	case FIFO: 
		cache = new FIFOLimitedMemoryCache();
	break;
	default: 
		log -> _Fatal("CustomDB::open::cacheType error\n");
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
		log -> _Fatal("CustomDB::open::factory error\n");
	}

	if(!factory)
		log -> _Fatal("CustomDB::open::new factory error\n");

	env = new Env(this);

	if(!env)
		log -> _Fatal("CustomDB::open::new Env error\n");

	if(env -> init() == 0)
		Log -> _Fatal("CustomDB::open::env open error\n");
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
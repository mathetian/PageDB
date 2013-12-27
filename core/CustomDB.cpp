#include "../include/CustomDB.h"
#include "../include/FIFOLimitedMemoryCache.h"
#include "../include/EHash.h"
#include "../include/CHash.h"

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
	
	if(cache)
	{
		delete cache;
		cache=NULL;
	}
}

bool CustomDB::open(const Options&option)
{
	this -> option = option;

	log = Log::GetInstance();
	log -> SetLogInfo(option.prefix, option.logLevel);

	switch(option.cacheOption.cacheType)
	{
	case FIFO: 
		cache = new FIFOLimitedMemoryCache();
	break;
	default: 
		log -> _Fatal("CustomDB::open::cacheType error\n");
		break;
	}

	if(cache == NULL)
		log -> _Fatal("CustomDB::open::new cache error\n");

	switch(option.factoryOption.factoryType)
	{
	case EHASH:
		factory = new ExtendibleHash();
		break;
	case CHASH:
		factory = new ChainHash();
		break;
	default:
		log -> _Fatal("CustomDB::open::factory error\n");
	}

	if(factory == NULL)
		log -> _Fatal("CustomDB::open::new factory error\n");

	if(init() == 0)
		log -> _Fatal("CustomDB::open::env open error\n");	
}

bool CustomDB::put(const string&key,const string&value)
{
	errorStatus = ERROR;
	if((cache -> get(key)).size() != 0) 
		log -> _Trace("CustomDB::put::exist in cache\n");
	else
	{
		if(!factory -> put(key,value))
			log -> _Warn("CustomDB::put::factory put error\n");
		else 
			errorStatus = SUCCE;
	}
	return errorStatus;
}

string CustomDB::get(const string&key)
{
	errorStatus = ERROR;
	string rs = cache -> get(key);
	if(rs.size() != 0) errorStatus = SUCCE;
	else
	{
		rs = factory -> get(key);
		if(rs.size() == 0) 
			log -> _Warn("CustomDB::get::factor get error\n");
		else errorStatus = SUCCE;
	}
	return rs;
}

bool CustomDB::remove(const string&key)
{
	errorStatus = ERROR;
	string rs = cache -> get(key);
	if(rs.size() != 0)
	{
		if((cache -> remove(key)) == 0)
		{
			log -> _Error("CustomDB::remove::cache remove error\n");
			return errorStatus;
		}
	}
	if((factory -> remove(key)) == 0)
		log -> _Error("CustomDB::remove::factory remove error\n");
	errorStatus = SUCCE;
	return errorStatus;
}

bool CustomDB::getError()
{
	return errorStatus;
}

bool CustomDB::init()
{
	factory -> 
}
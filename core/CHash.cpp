#include "CHash.h"

Chain::Chain()
{
	ssMap.clear();
}

Chain::~Chain()
{

}

bool     Chain::put(const string&key,const string&value)
{
	if(ssMap.find(key) == ssMap.end())
	{
		ssMap[key] = value;
		return true;
	}
	return false;
}	

string   Chain::get(const string&key)
{
	if(ssMap.find(key) == ssMap.end())
		return "";
	return ssMap[key];
}

bool     Chain::remove(const string&key)
{
	if(ssMap.find(key) == ssMap.end())
		return false;
	ssMap.erase(key);
	return true;
}

ChainHash::ChainHash(int chainCount,HASH hashFunc)
{
	this -> chainCount = chainCount;
	this -> hashFunc = hashFunc;
	header =  vector <Chain*> (chainCount,new Chain());
}

ChainHash::~ChainHash()
{
	vector <Chain*>::iterator itList= header.begin();
	for(;itList != header.end();itList++)
	{
		chain = *itList;
		if(chain)
			delete chain;
		chain = NULL;
	}
}

bool ChainHash::put(const string&key,const string&value)
{
	chain = getChain(key);
	return chain -> put(key,value);
}

string ChainHash::get(const string&key)
{
	chain = getChain(key);
	return chain -> get(key);
}

bool ChainHash::remove(const string&key)
{
	chain =  getChain(key);
	return chain -> remove(key);
}

Chain * ChainHash::getChain(const string&key)
{
	curId = hashFunc(key) % chainCount;
	return header.at(curId);
}

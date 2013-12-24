#include "EHash.h"

Page::Page() : d(0)
{
	ssMap.clear();
}

Page::~Page()
{
}

bool Page::full()
{
	return ssMap.size() > PAGESIZE ? 1:0;
}

void Page::put(const string&key,const string&value)
{
	if(ssMap.find(key) == ssMap.end())
  		ssMap[key] = value;
}

string Page::get(const string&key)
{
	if(ssMap.find(key) == ssMap.end()) 
		return "";
	else 
		return ssMap[key];
}

bool Page::remove(const string&key)
{
	if(ssMap.find(key) == ssMap.end()) 
		return 0;
  ssMap.erase(key);
  return true;
}

ExtendibleHash::ExtendibleHash(HASH hashFunc)
{
	pages.push_back(new Page());
	this -> hashFunc = hashFunc;
	this -> gd = 0;
}

ExtendibleHash::~ExtendibleHash()
{
	return;
    int index = 0;
    for(;index < pages.size();index++)
    {
      page = pages[index];
      if(page)
        delete page;
      pages[index] = NULL;
    }
}

bool ExtendibleHash::put(const string&key,const string&value)
{
	page = getPage(key);
  	
  	if(page -> full() && page -> d == gd)
  	{
		this -> gd++;
		int oldSize = pages.size();
		for(int i = 0;i < oldSize;i++)
		    pages.push_back(pages.at(i));
    }
  	
	if(page -> full() && page -> d < gd)
	{
    	page -> put(key,value);
		Page * p1 = new Page();
		Page * p0 = new Page();
  		
  		map <string,string>& ssMap = page -> ssMap;

  		map <string,string>::iterator stMap = ssMap.begin();
  		
  		for(;stMap != ssMap.end();stMap++)
  		{
  			int id = hashFunc(stMap -> first);
  			string str = stMap -> second;
      		int flag = (id & ((1 << gd) - 1));
  			if(((flag >> (page -> d)) & 1) == 1)
  				p1 -> put(stMap -> first,stMap -> second);
  			else
  				p0 -> put(stMap -> first,stMap -> second);
  		}
  		
    	int id;
    	for(id = 0;id < pages.size();id++)
    	{
        	if(pages.at(id) == page)
        	{
            	if(((id >> (page -> d)) & 1) == 1)
              		pages[id] = p1;
            	else
              		pages[id] = p0;
        	}
    	}
  		p1 -> d = (page -> d) + 1; p0 -> d = (page -> d) + 1; 
    	delete page;
    	page = NULL;
	}
  	else
   		page -> put(key,value);
  	
  	return true;
}

string ExtendibleHash::get(const string&key)
{
	page = getPage(key);
  	return page -> get(key);
}

bool ExtendibleHash::remove(const string&key)
{
	page = getPage(key);
    return page -> remove(key);
}

Page * ExtendibleHash::getPage(string key)
{
	int hashVal = hashFunc(key);
	curId = hashVal & ((1 << gd)-1);
	return pages.at(curId);
}

int defaultHashFunc(const string&str)
{
  int index;
  int value = 0x238F13AF * key.size();
  for(index = 0;index < key.size();index++)
      value = (value + (key.at(index) << (index*5 % 24))) & 0x7FFFFFFF;
  return value;
}
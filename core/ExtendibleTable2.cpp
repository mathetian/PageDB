#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <time.h>
#include <map>
#include <string>
#include <stdlib.h>
using namespace std;

#define PAGESIZE 25

typedef int (*HASH)(string key);

class ExtendibleTable;

class Page{
public:
	  Page():d(0) {ssMap.clear();}
  ~ Page()      {}
  	
    void insert(string key,string value)
  	{
  		if(ssMap.find(key) == ssMap.end())
  			ssMap[key] = value;
  	}

private:
  	bool full() { return ssMap.size() > PAGESIZE ? 1:0; }
  	int  getD() { return d; }
  	string get(string key)
  	{
  		if(ssMap.find(key)==ssMap.end()) return "";
  		else return ssMap[key];
  	}
private:
	int d;
	map <string,string> ssMap;
	friend class ExtendibleTable;
};

class ExtendibleTable{
public:
	ExtendibleTable(HASH hashFunc)
	{
		pages.push_back(new Page());
		this->hashFunc = hashFunc;
		gd=0;
	}
 
  ~ ExtendibleTable()
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

  	Page * getPage(string key)
  	{	
  		int pos = hashFunc(key);
  		curId = pos & ((1 << gd)-1);
  		return pages.at(curId);
  	}

  	bool   put(string key,string value)
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
        page -> insert(key,value);
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
	  				p1->insert(stMap -> first,stMap -> second);
	  			else
	  				p0->insert(stMap -> first,stMap -> second);
	  		}
	  		vector<Page*>::iterator itList = pages.begin();
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
        page -> insert(key,value);
      return true;
  	}

  	string get(string key)
  	{
  		page = getPage(key);
  		return page -> get(key);
  	}
private:
	int gd; Page * page;
	vector <Page*> pages;
	HASH hashFunc;
	int curId;
};

string convert(int v)
{
	stringstream ss;
	ss<<v;
	return ss.str();
}

int func1 (string key)
{
  int index;
  int value = 0x238F13AF * key.size();
  for(index = 0;index < key.size();index++)
  {
  	value = (value + (key.at(index) << (index*5 % 24))) & 0x7FFFFFFF;
  }
  return value;
}

#define LEN 10000

int main()
{
	ExtendibleTable etables(func1);
	int i;
	srand(time(0));
	vector<int> testData(LEN,0);
	for(i = 0; i< LEN;i++)
		testData[i]=rand();
	cout<<"hello"<<endl;
	for(i=1;i<LEN;i++)
		etables.put(convert(i),convert(testData.at(i)));
	for(i=1;i < LEN;i++)
		cout<<i<<" "<<etables.get(convert(i))<<endl;
	return 0;
}

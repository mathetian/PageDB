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
  		cout<<ssMap.size()<<endl;
  		map <string,string> ::iterator stMap = ssMap.begin();
  		int i=ssMap.size();
  		if(i==0) ssMap.clear();
  		for(;stMap != ssMap.end()&&i>0;stMap++,i--)
  		{
  		//	cout<<"1"<<endl;
  	//		cout<<"("<<stMap->first<<" "<<stMap->second<<")";
  		}
  		cout<<endl;
  		if(ssMap.find(key) == ssMap.end())
  			ssMap[key] = value;
  		/*cout<<key<<" "<<value<<endl;*/
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
 
 /* ~ ExtendibleTable()
 	{
 		vector<Page*>::iterator itList=pages.begin();
 		for(;itList < pages.end();itList++)
 		{
 			page = *itList;
 			if(page) 
 				delete page;
 			page = NULL;
 		}	
 	}*/
  	
  	Page * getPage(string key)
  	{	
  		cout<<"here?1"<<endl;
  		int pos = hashFunc(key);cout<<"here?2"<<endl;
  		curId = pos & ((1 << gd)-1);
  		cout<<"here?3"<<endl;
  		return pages.at(curId);
  	}

  	bool   put(string key,string value)
  	{
  		page = getPage(key);
  		cout<<key<<endl;
  		if(!page) cout<<"what?"<<endl;
  		if(page -> full())
  		{
  			this -> gd++;
  			int oldSize=pages.size();
  			pages.resize(2*pages.size());
  			for(int i = oldSize;i<pages.size();i++)
  				if(!pages.at(i)) pages[i] = new Page();
  		}
  		cout<<key<<endl;
  		cout<<(page->ssMap).size()<<endl;
  		page -> insert(key,value);
  		cout<<key<<endl;

  		if(page -> full())
  		{
  			  		cout<<key<<endl;

  			Page * p1 = new Page();
  			Page * p0 = new Page();
	  		
	  		map <string,string>& ssMap = page -> ssMap;

	  		map <string,string>::iterator stMap = ssMap.begin();

	  		for(;stMap != ssMap.end();stMap++)
	  		{
	  			int id = hashFunc(stMap -> first);
	  			string str = stMap -> second;
	  			if(((id & ((1 << gd) - 1)) >> (page -> d)) == 1)
	  				p1->insert(stMap -> first,stMap -> second);
	  			else
	  				p0->insert(stMap -> first,stMap -> second);
	  		}
	  		pages[curId] = p1;
	  		pages[curId + (pages.size() >> 1)] = p0;
	  		p1 -> d = gd; p0 -> d = gd; 
  		}
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
	for(i=0;i<LEN;i++)
		etables.put(convert(i),convert(testData.at(i)));
	for(i=0;i < LEN;i++)
		cout<<i<<" "<<etables.get(convert(i))<<endl;
	return 0;
}

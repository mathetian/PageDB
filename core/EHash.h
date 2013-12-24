#ifndef _E_TABLE_H
#define _E_TABLE_H
#define PAGESIZE 25

#include <string>
#include <map>
#include <vector>
using namespace std;

#include "Factory.h"

#define PAGESIZE 25

typedef int (*HASH)(const string&key);

extern int defaultHashFunc(const string&str);

class ExtendibleHash;

class Page{
public:
	Page();
  ~ Page();

private:	
    bool   full();
    void   put(const string&key,const string&value);
    string get(const string&key);
    bool   remove(const string&key);

private:
	int d;
	map <string,string> ssMap;
	friend class ExtendibleHash;
};

class ExtendibleHash : public Factory{
public:
	  ExtendibleHash(HASH hashFunc = defaultHashFunc);
  ~ ExtendibleHash();

public:
  	bool     put(const string&key,const string&value);
  	string   get(const string&key);
    bool     remove(const string&key);

private:
    Page  *  getPage(string key);

private:
	int          gd; 
  int       curId;
  HASH   hashFunc;
  Page   *   page;
	vector <Page*> pages;
};

#endif
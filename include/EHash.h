#ifndef _E_TABLE_H
#define _E_TABLE_H
#define PAGESIZE 25

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
using namespace std;

#include <sys/types.h>
#include "Factory.h"
#include "Slice.h"

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
  Slice keys[PAGESIZE];
  Slice values[PAGESIZE];
	friend class ExtendibleHash;
  int curNum;

public:

  istream & operator >> (istream &in)
  {
     in >> d >> curNum;
     int index;
     for(index = 0;index < curNum;index++)
       in >> keys[index] >> values[index];
  }

  ostream & operator << (ostream & os)
  {
    os << d << curNum;
    int index;
    for(index = 0;index < curNum;index++)
      os << keys[index] << values[index];
  }
};

class ExtendibleHash : public Factory{
public:
	  ExtendibleHash(HASH hashFunc = defaultHashFunc);
  ~ ExtendibleHash();

public:
  	bool     put(const string&key,const string&value);
  	string   get(const string&key);
    bool     remove(const string&key);
    bool     init(const string&filename);
    
private:
    Page  *  getPage(string key);

private:
	int            gd; 
  int            curId;
  HASH           hashFunc;
  Page   *       page;
	vector <Page*> pages;
  vector <int>   dirIdx;
  ifstream       ifs;
  ofstream       ofs;

public:
   void read(const ifstream &in)
   {

   }
   void write(const ofstream &out)
   {

   }
};

#endif
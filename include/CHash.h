#ifndef _C_HASH_H
#define _C_HASH_H

#include <list>
#include <vector>
using namespace std;

#include <fstream>
using namespace std;

#include "Factory.h"

typedef int (*HASH)(const string&key);

extern int defaultHashFunc(const string&str);

#define PAGESIZE 25
#define SINT     sizeof(int)
#define SEBLOCK  sizeof(EmptyBlock)
#define SELEM    sizeof(Elem)

typedef struct _tEmptyEle{
  int   pos, size;
  _tEmptyEle()
  { pos = size = -1;  }
}EmptyEle;

class EmptyBlock{
public:
    EmptyBlock();
   ~EmptyBlock();
  bool checkSuitable(int size, int & pos);
  void newBlock(int size);

public:
  int      curNum;
  int      nextBlock;
  EmptyEle eles[PAGESIZE];
};

typedef struct _tELEM{
  int nextOffset, keySize;
  int valueSize, hashVal;
  _tELEM()
  {
    nextOffset = -1;
    keySize = valueSize = 0;
  }
  _tELEM(int a,int b,int c)
  {
    nextOffset = a;
    keySize = b;
    valueSize = c;
  }
}Elem;

class ChainHash;

class Chain{
public:
	  Chain(ChainHash * cHash, int defaultFirstOffset = -1);
	~ Chain();

private:
	bool     put(const string&key,const string&value, int hashVal);
  string   get(const string&key, int hashVal);
  bool     remove(const string&key, int hashVal);
  bool     check(const string&key, int hashVal);

private:
  friend   class ChainHash;
  ChainHash * cHash;
private:
  int       firstoffset;
};

class ChainHash : public Factory{
public:
      ChainHash(int chainCount = 100, HASH hashFunc = defaultHashFunc);
    ~ ChainHash();

public:
  	bool     put(const string&key,const string&value);
  	string   get(const string&key);
    bool     remove(const string&key);
    bool	   init(const string&filename);

private:
    void     cycle(int offset, int size);

private:
    void     writeToFile();
    void     readFromFile();
    int      findSuitable(int size);

private:
	HASH      hashFunc;
  fstream   datfs;
  vector <Chain*> headers; 
  friend  class  Chain;

private:
	int      chainCount, fb;
	vector <int> entries;
};

#endif
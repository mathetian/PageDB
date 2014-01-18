#ifndef _E_TABLE_H
#define _E_TABLE_H
#define PAGESIZE 25

#include <string>
#include <vector>
#include <fstream>
using namespace std;

#include "Factory.h"
#include "../helpers/BufferPacket.h"

#define PAGESIZE  25
#define SINT      sizeof(int)
#define SPAGE     sizeof(Page)
#define SEEBLOCK  sizeof(EEmptyBlock)
#define SPELEMENT sizeof(PageElement)

class EEmptyEle
{ public: 
    EEmptyEle(): pos(-1), size(-1) { }
    int  pos, size;
};

class EEmptyBlock
{
public:
    EEmptyBlock() : curNum(0), nextBlock(-1) { }

public:
    bool        checkSuitable(int size, int & pos);
    EEmptyBlock split();

public:
    int       curNum;
    int       nextBlock;
    EEmptyEle eles[PAGESIZE];
};

class Page;
class ExtendibleHash;

class PageElement
{
public:
    PageElement(): m_hashVal(-1), m_datPos(-1), m_keySize(-1), m_datSize(-1){}

private:
    int   m_hashVal, m_datPos;
    int   m_keySize, m_datSize;

    friend ostream & operator << (ostream & os, PageElement & e);
    friend class Page;
    friend class ExtendibleHash;
};

inline  ostream & operator << (ostream & os, PageElement & e)
{
    os << e.m_hashVal << " "<< e.m_datPos << " "<< e.m_keySize <<" "<<e.m_datSize<<endl;
    return os;
}



typedef int (*HASH)(const Slice & key);

class Page
{
public:
    Page(ExtendibleHash * eHash) : d(0),curNum(0) { this -> eHash = eHash; }

public:
    BufferPacket getPacket();
    void         setByBucket(BufferPacket & packet);
    int          getSize() { return SINT*2 + sizeof(elements);}

private:
    bool   full() { return curNum >= PAGESIZE; }
    bool   put(const Slice & key,const Slice & value, int hashVal);
    Slice  get(const Slice & key, int hashVal);
    bool   remove(const Slice & key, int hashVal);

private:
    friend class ExtendibleHash;
    ExtendibleHash * eHash;

private:
    /**
       Asscoiate with the file,
       Use PAGESIZE + 5 to avoid some special(boundary) situation
    **/
    int d, curNum;
    PageElement elements[PAGESIZE + 5];
};

class ExtendibleHash : public Factory
{
public:
    ExtendibleHash(HASH hashFunc = defaultHashFunc) :\
        hashFunc(hashFunc), gd(0), pn(1), fb(-1) { }
    ~ExtendibleHash() { if(idxfs) idxfs.close(); if(datfs) datfs.close();}

public:
    bool     put(const Slice & key,const Slice & value);
    Slice    get(const Slice & key);
    bool     remove(const Slice & key);
    bool     init(const char * filename);

private:
    void     recycle(int offset, int size);
    void     writeToIdxFile(); /**Initialization, so write something into file**/
    void     readFromFile();/**Read the Index information**/
    int      findSuitableOffset(int size);

private:
    HASH      hashFunc;
    bool      updated, eupdated; /**two files update status**/
    fstream   idxfs, datfs;
    friend class Page;

private:
    /**Need read from file**/
    int           gd, pn, fb;
    vector <int>  entries; /**Page entries, just offset for each page**/
};

#endif
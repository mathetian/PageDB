#ifndef _Factory_IMPL_H
#define _Factory_IMPL_H

#include <vector>
#include <fstream>
using namespace std;

#include <sys/stat.h>

#include "Factory.h"
#include "BufferPacket.h"
#include "HashFunction.h"

#define PAGESIZE 50
#define SINT     sizeof(int)

#define SCEBLOCK sizeof(CEmptyBlock)
#define SELEM    sizeof(CElement)

typedef uint32_t (*HASH)(const Slice & key);

class CEmptyBlock
{
public:
    CEmptyBlock() : curNum(0), nextBlock(-1) { }
    ~CEmptyBlock() { }
    bool checkSuitable(int size, int & pos);
    void newBlock(int size);
    CEmptyBlock split();
    
public:
    class CEmptyEle
    { public: 
        CEmptyEle(): pos(-1), size(-1) { }
        int  pos, size;
    };

public:
    int       curNum;
    int       nextBlock;
    CEmptyEle eles[PAGESIZE];
};

class Chain;
class ChainHash;

class CElement
{
public:
    CElement(int nextOffset = -1, int keySize = 0, int valueSize = 0, uint32_t hashVal = 0) \
        : nextOffset(nextOffset), keySize(keySize), valueSize(valueSize), hashVal(hashVal) { }

private:
    int nextOffset, keySize;
    uint32_t valueSize, hashVal;

    friend class Chain;
    friend class ChainHash;
};

class Chain
{
public:
    Chain(ChainHash * cHash, int defaultFirstOffset = -1) : cHash(cHash),\
             firstoffset(defaultFirstOffset) { }
             
private:
    bool     put(const Slice & key,const Slice & value, uint32_t hashVal);
    Slice    get(const Slice & key, uint32_t hashVal);
    bool     remove(const Slice & key, uint32_t hashVal);
    bool     check(const Slice & key, uint32_t hashVal); /**???**/

private:
    friend   class ChainHash;
    ChainHash * cHash;

private:
    int       firstoffset;
};

class ChainHash : public Factory
{
public:
    ChainHash(int chainCount = 100, HASH hashFunc = MurmurHash3) :\
        chainCount(chainCount), hashFunc(hashFunc) { }
    virtual ~ChainHash();

public:
    bool     put(const Slice & key,const Slice & value);
    Slice    get(const Slice & key);
    bool     remove(const Slice & key);
    bool	 init(const char * filename);
    void     removeAll(const char * filename) { }
    void     dump()
    {
        //Todo list
    }
    
private:
    void     recycle(int offset, int size);

private:
    void     writeToFile();
    void     readFromFile();
    int      findSuitableOffset(int size);

private:
    HASH      hashFunc;
    fstream   datfs;
    vector <Chain*> headers; /**??**/
    friend  class  Chain;

private:
    int      chainCount, fb;
    vector <int> entries;
};

#define SPAGE     sizeof(Page)
#define SEEBLOCK  sizeof(EEmptyBlock)
#define SPELEMENT sizeof(PageElement)

class EEmptyBlock
{
public:
    EEmptyBlock() : curNum(0), nextBlock(-1) { }

public:
    bool        checkSuitable(int size, int & pos);
    EEmptyBlock split();
    
public:
    class EEmptyEle
    { public: 
        EEmptyEle(): pos(-1), size(-1) { }
        int  pos, size;
    };

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
    PageElement(): m_hashVal(0), m_datPos(-1), m_keySize(-1), m_datSize(-1){}
    void  clear()
    {
        m_hashVal = -1; m_datPos = -1; m_keySize = -1; m_datSize = -1;
    }
    
private:
    uint32_t   m_hashVal, m_datPos;
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
    bool   put(const Slice & key,const Slice & value, uint32_t hashVal);
    Slice  get(const Slice & key, uint32_t hashVal);
    bool   remove(const Slice & key, uint32_t hashVal);

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
    ExtendibleHash(HASH hashFunc = MurmurHash3) :\
        hashFunc(hashFunc), gd(0), pn(1), fb(-1) { }
    virtual ~ExtendibleHash() 
    { 
        if(idxfs) idxfs.close(); 
        if(datfs) datfs.close();
    }

public:
    bool     put(const Slice & key,const Slice & value);
    Slice    get(const Slice & key);
    bool     remove(const Slice & key);
    bool     init(const char * filename);
    void     dump();
    void     removeAll(const char * filename)
    {  
        string sfilename(filename, filename + strlen(filename));
        string idxName = sfilename + ".idx";
        string datName = sfilename +  ".dat";

        idxName = "rm " + idxName;
        datName = "rm " + datName;
        system(idxName.c_str());
        system(datName.c_str());
    }

private:
    void     recycle(int offset, int size);
    void     writeToIdxFile(); /**Initialization, so write something into file**/
    void     readFromFile();/**Read the Index information**/
    int      findSuitableOffset(int size);

    void     printThisPage(Page * page);

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
#ifndef _CHASH_IMPL_H
#define _CHASH_IMPL_H

#include "../helpers/HashFunction.h"

#include "DBInternal.h"

namespace customdb
{

#define SCEBLOCK sizeof(CEmptyBlock)
#define SELEM    sizeof(CElement)

class ChainDB;
class ChainTable;
class ChainElement;
class CEmptyBlock;

class ChainEmptyBlock
{
public:
    ChainEmptyBlock();

public:
    bool        checkSuitable(int size, int & pos);
    void        newBlock(int size);
    ChainEmptyBlock split();

public:
    class CEmptyEle
    {
    public:
        CEmptyEle(): pos(-1), size(-1) { }
        int  pos, size;
    };

private:
    int       curNum;
    int       nextBlock;
    CEmptyEle eles[PAGESIZE];
};

class ChainElement
{
public:
    ChainElement(int nextOffset = -1, int keySize = 0,\
                 int valueSize = 0, uint32_t hashVal = 0);

private:
    uint32_t nextOffset, keySize;
    uint32_t valueSize , hashVal;
};

class ChainTable
{
public:
    ChainTable(ChainDB * cHash, int defaultFirstOffset = -1);

private:
    bool     put(const Slice & key,const Slice & value, uint32_t hashVal);
    Slice    get(const Slice & key, uint32_t hashVal);
    bool     remove(const Slice & key, uint32_t hashVal);
    bool     check(const Slice & key, uint32_t hashVal);

private:
    ChainDB * cHash;
    int       firstoffset;
};

class ChainDB : public DBInternal
{
public:
    ChainDB(int chainCount = 100, HASH hashFunc = MurmurHash3);
    virtual ~ChainDB();

public:
    bool     put(const Slice & key,const Slice & value);
    Slice    get(const Slice & key);
    bool     remove(const Slice & key);
    bool	 init(const char * filename);
    void     removeAll(const char * filename) { }
    void     dump();
    void     fflush();
    void     runBatch(const WriteBatch * pbatch);
    void     runBatch2(const WriteBatch * pbatch);
    void     write(WriteBatch* pbatch);
    void     compact();

private:
    void     recycle(int offset, int size);
    void     writeToFile();
    void     readFromFile();
    int      findSuitableOffset(int size);

private:
    HASH      hashFunc;
    fstream   datfs;
    vector <ChainTable*> headers;

private:
    int      chainCount, fb;
    vector <int> entries;
};

};
#endif
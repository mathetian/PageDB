#ifndef _CHASH_IMPL_H
#define _CHASH_IMPL_H

#include "DBInternal.h"

namespace customdb
{

#define SCEBLOCK sizeof(CEmptyBlock)
#define SELEM    sizeof(CElement)

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
    {
    public:
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
:
    nextOffset(nextOffset), keySize(keySize), valueSize(valueSize), hashVal(hashVal) { }

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

    void     fflush()
    {
        //Todo list
    }

    void    runBatch(const WriteBatch * pbatch)
    {
        //Todo list
    }

    void    runBatch2(const WriteBatch * pbatch)
    {
        //Todo list
    }

    void    write(WriteBatch* pbatch)
    {
        //Todo list
    }

    void    compact()
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

};
#endif
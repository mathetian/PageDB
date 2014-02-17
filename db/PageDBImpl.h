#ifndef _EHASH_IMPL_H
#define _EHASH_IMPL_H

#include <queue>
using std::deque;

#include "../helpers/BufferPacket.h"
#include "../include/Batch.h"
#include "../utils/Thread.h"
#include "../utils/TimeStamp.h"
#include "../helpers/HashFunction.h"
using namespace utils;

#include "DBInternal.h"

namespace customdb
{

#define SPAGE     sizeof(2*SINT+(PAGESIZE+5)*sizeof(PageElement))
#define SPELEMENT sizeof(PageElement)
#define SEEBLOCK  sizeof(PageEmptyBlock)

class PageDB;
class PageTable;
class PageElement;
class PageCache;
class PageEmptyBlock;

class PageEmptyBlock
{
public:
    PageEmptyBlock();

public:
    bool           checkSuitable(int size, int & pos);
    PageEmptyBlock split();

public:
    typedef struct PageEmptyEle_t
    {
        PageEmptyEle_t(): pos(-1), size(-1) { }
        int  pos, size;
    } PageEmptyEle_t;

public:
    int            curNum;
    int            nextBlock;
    PageEmptyEle_t eles[PAGESIZE];
};

class PageElement
{
public:
    PageElement();
    void  clear();/**?**/

private:
    uint32_t   m_hashVal, m_datPos;
    uint32_t   m_keySize, m_datSize;
};

class PageTable
{
public:
    PageTable(PageDB * eHash);

public:
    BufferPacket getPacket();
    void         setByBucket(BufferPacket & packet);

private:
    bool   full();
    bool   put(const Slice & key,const Slice & value, uint32_t hashVal);
    Slice  get(const Slice & key, uint32_t hashVal);
    bool   remove(const Slice & key, uint32_t hashVal);
    void   replaceQ(const Slice & key, const Slice & value, uint32_t hashVal, int offset);

private:
    void        addElement(PageElement element);
    PageElement getElement(int index);
    PageElement getElement2(int index);
    int         getCurNum() const;
    int         getD() const;
    void        setCurNum(int num);
    void        setD(int d);

private:
    PageDB * eHash;

private:
    volatile int d, curNum;
    PageElement elements[PAGESIZE + 5];
    Log * m_log;
};

class PageDB : public DBInternal
{
public:
    PageDB(HASH hashFunc = MurmurHash3);
    virtual ~PageDB();

public:
    bool     put(const Slice & key,const Slice & value);
    Slice    get(const Slice & key);
    bool     remove(const Slice & key);
    bool     init(const char * filename);
    void     dump();
    void     removeAll(const char * filename);
    void     fflush();
    void     runBatch(const WriteBatch * pbatch);
    void     runBatch2(const WriteBatch * pbatch);
    void     compact();
    void     write(WriteBatch* pbatch);

private:
    void     recycle(int offset, int size);
    void     writeToIdxFile();
    void     readFromFile();
    int      findSuitableOffset(int size);
    void     printThisPage(PageTable * page);
    void     fullAddLocalD(int cur, uint64_t num, uint64_t pos1, uint64_t pos2, uint64_t od);

private:
    HASH        hashFunc;
    bool        updated, eupdated;
    fstream     idxfs, datfs;
    PageCache * pcache;

private:
    struct Writer;
    WriteBatch* BuildBatchGroup(Writer ** last_writer);

private:
    deque<Writer*> m_writers;
    WriteBatch *   m_tmpBatch;
    Mutex          m_mutex;

    string         idxName;
    string         datName;

private:
    Mutex          datLock;
    Mutex          cacheLock;
    RWLock         globalLock;
    Mutex          tmplock;
    Mutex          cacheElemLock[CACHESIZE];

private:
    volatile int  gd, pn, fb;
    vector <uint64_t>  entries;

    const uint64_t MOD;
};

struct CacheElem_t
{
    PageTable * page;
    uint32_t entry;
    bool   updated;
    CacheElem_t() : page(NULL)
    {
        reset();
    }
    void reset()
    {
        if(page != NULL) delete page;
        page = NULL;
        entry = -1;
        updated = false;
    }
};

typedef struct CacheElem_t CacheElem;

class PageCache
{
public:
    PageCache(PageDB * eHash);
    ~PageCache();

private:
    void        free();
    void        freeWithLock();
    PageTable * find(uint32_t addr, int & index);
    int         putInto(PageTable * page, int addr);
    void        setUpdated(int index);
    int         findLockable(PageTable * page, uint32_t addr);
    void        fflush();
    void        fflushWithLock();

private:
    void        resetWithDatLock(int index);
    void        reset(int index);

private:
    PageDB *  eHash;
    CacheElem cacheElems[CACHESIZE];
    int cur;
};

};
#endif
#ifndef _EHASH_IMPL_H
#define _EHASH_IMPL_H

#include <queue>
using std::deque;

#include "BufferPacket.h"
#include "Batch.h"
#include "Thread.h"
#include "TimeStamp.h"
#include "HashFunction.h"
#include "Utils.h"
#include "FileModule.h"
using namespace utils;

#include <sys/stat.h>

#include "DBInternal.h"


namespace customdb
{

#define SPAGETABLE  (2*SINT+(PAGESIZE+5)*sizeof(PageElement))
#define SPELEMENT   sizeof(PageElement)
#define SEEBLOCK    sizeof(PageEmptyBlock)

class PageEmptyBlock;
class PageElement;
class PageTable;
class PageDB;
class PageCache;

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
    } PageEmptyEle;

public:
    int            curNum;
    int            nextBlock;
    PageEmptyEle eles[PAGESIZE];
};

extern ostream & operator << (ostream & os, PageElement & e);

class PageElement
{
public:
    PageElement();
    void  clear();/**?**/

private:
    uint32_t   m_hashVal, m_datPos;
    uint8_t    m_keySize, m_datSize;
    friend ostream & operator << (ostream & os, PageElement & e);

private:
    friend class PageTable;
    friend class PageDB;
};

class PageTable
{
public:
    PageTable(PageDB * db);

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
    void        addElement(const PageElement & element);
    PageElement getElement(int index);
    PageElement getElement2(int index);
    int         getCurNum() const;
    int         getD() const;
    void        setCurNum(int num);
    void        setD(int d);

private:
    PageDB * db;

private:
    int d, curNum;
    PageElement elements[PAGESIZE + 5];
    Log * m_log;

private:
    friend class PageDB;
};

class PageDB : public DBInternal
{
private:
    typedef pair<Slice, Slice> Node;

public:
    PageDB(HASH hashFunc = MurmurHash3);
    virtual ~PageDB();

public:
    bool     put(const Slice & key,const Slice & value);
    Slice    get(const Slice & key);
    bool     remove(const Slice & key);
    bool     init(const char * filename);
    void     dump();
    void     removeDB(const char *filename);
    void     fflush();
    void     runBatch(const WriteBatch * pbatch);
    void     runBatchParallel(const WriteBatch * pbatch);
    void     compact();
    void     write(WriteBatch* pbatch);

private:
    void     recycle(int offset, int size);
    void     writeToIdxFile();
    void     readFromFile();
    int      findSuitableOffset(int size);
    void     printThisPage(PageTable * page);
    void     fullAddLocalD(int cur, uint64_t num, uint64_t pos1, uint64_t pos2, uint64_t od);
    void     reOpenDB();

private:
    HASH        hashFunc;
    bool        updated, eupdated;
    RandomFile  m_idxfile;
    RandomFile  m_datfile;
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
    const  uint64_t MOD;
private:
    Mutex          datLock;
    Mutex          cacheLock;
    RWLock         globalLock;
    Mutex          tmplock;
    Mutex          cacheElemLock[CACHESIZE];

private:
    volatile int  gd, pn, fb;
    vector <uint64_t>  entries;

private:
    friend class PageCache;
    friend class PageTable;
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
    PageCache(PageDB * db);
    ~PageCache();

private:
    void        free();
    void        freeWithLock();
    PageTable * find(uint32_t addr, uint32_t & index);
    int         putInto(PageTable * page, int addr);
    void        setUpdated(int index);
    int         findLockable(PageTable * page, uint32_t addr);
    void        fflush();
    void        fflushWithLock();

private:
    void        resetWithDatLock(int index);
    void        reset(int index);

private:
    PageDB *  db;
    CacheElem cacheElems[CACHESIZE];
    int cur;
    friend class PageDB;
};

};
#endif
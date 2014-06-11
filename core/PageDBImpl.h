// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _EHASH_IMPL_H
#define _EHASH_IMPL_H

#include "TickTimer.h"
#include "FileModule.h"
#include "HashFunction.h"
#include "Multithreading.h"
using namespace utils;

#include "DBInternal.h"
#include "BufferPacket.h"

/**
** Macro for size
**/

#define SPAGETABLE  (2*SINT+(PAGESIZE+5)*sizeof(PageElement))
#define SPELEMENT   sizeof(PageElement)
#define SEEBLOCK    sizeof(PageEmptyBlock)
#define MOD         ((1ull << 56) - 1)
/**
** PageDBImpl inherits from DBInternal and include many classes
**/
namespace customdb
{
/**
** There are several sub-classes in PageDB
**
** PageEmptyBlock is responsibly for EmptyBlock
** PageElement store the address of key/value
** PageTable is the archive of PageElement
** PageDB is constructed by PageTable
** PageCache stores the Page visited recently.
**/
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

private:
    typedef struct PageEmptyEle_t
    {
        PageEmptyEle_t(): pos(-1), size(-1) { }
        int  pos, size;
    } PageEmptyEle;

private:
    int            m_curNum;
    int            m_nextBlock;
    PageEmptyEle   m_eles[PAGESIZE];
};

class PageElement
{
public:
    PageElement();

public:
    void  clear();

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

class PageDB : public DBInternal, public Noncopyable
{
private:
    typedef pair<Slice, Slice> Node;

public:
    PageDB(HashFunc hashFunc = MurmurHash3);
    virtual ~PageDB();

public:
    /**
    ** Layer 1
    **/
    bool     open(const string &filename);
    bool     close();
    bool     put(const Slice & key,const Slice & value);
    Slice    get(const Slice & key);
    bool     remove(const Slice & key);
    /**
    ** Layer 2
    **/
    void     put(const WriteBatch * pbatch);
    void     write(WriteBatch* pbatch);
    void     runBatchParallel(const WriteBatch * pbatch);
    /**
    ** Layer 3
    **/
    void     sync();
    void     dump();
    void     compact();

private:
    /**
    ** Internal functions
    **/
    void     recycle(int offset, int size);
    /**
    ** Update Index information into idxfile
    **/
    void     writeToIdxFile();
    void     readFromFile();
    int      findSuitableOffset(int size);
    void     printThisPage(PageTable * page);
    void     fullAddLocalD(int cur, uint64_t num, uint64_t pos1, uint64_t pos2, uint64_t od);

private:
    struct Writer;
    WriteBatch* BuildBatchGroup(Writer ** last_writer);

    deque<Writer*> m_writers;
    WriteBatch *   m_tmpBatch;
    Mutex       m_writelock;

private:
    HashFunc    m_HashFunc;
    /**
    ** In PageDB, there are four kinds of files
    **
    ** m_idxfile.idx: index file (first layer)
    ** m_datfile.dat: data file (include datafile)
    ** tmpfiles.idx .dat: used during compacting
    ** logfiles.log: log files
    **/
    RandomFile  m_idxfile;
    RandomFile  m_datfile;
    /**
    ** Each time, it will check whether page exist in m_cache
    ** Then schedule different policy
    **/
    PageCache   *m_cache

    /**
    ** We need three locks to sync operations in `PageDB`
    ** m_datLock, operation on datfile
    ** m_cacheLock, operation on cache
    ** m_globalLock, Read-Write lock
    **/
    Mutex       m_datLock;
    Mutex       m_cacheLock;
    RWLock      m_globalLock;
    /**
    ** lock for cache element in cache
    **/
    Mutex       m_cacheElemLock[CACHESIZE];

private:
    /**
    ** global counter
    ** gd: global distance
    ** pn: maxinum number of pages
    ** fb: first empty block
    **/
    volatile int  m_gd, m_pn, m_fb;
    /**
    ** The index of first layer
    **/
    vector <uint64_t>  m_entries;
    /**
    ** DB_filename_prefix
    **/
    string m_prefix;
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
// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _EHASH_IMPL_H
#define _EHASH_IMPL_H

#include "TickTimer.h"
#include "FileModule.h"
#include "Noncopyable.h"
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

/**
** PageEmptyBlock is the archive of EmptyElement
** EmptyElement represent the position of element
**/
class PageEmptyBlock
{
public:
    PageEmptyBlock();

public:
    /**
    ** Find suitable index
    ** Return true, if exist suitable element
    ** Return false, if not exist
    **/
    bool           find(int size, int & pos);
    /**
    ** Split the EmptyBlock into two blocks
    **/
    PageEmptyBlock split();

private:
    class PageEmptyEle
    {
    public:
        PageEmptyEle(): m_pos(-1), m_size(-1) { }
        int  m_pos, m_size;
    };
private:
    int            m_curNum;
    int            m_nextBlock;
    PageEmptyEle   m_eles[PAGESIZE];
    friend class   PageDB;
};

/**
** PageElement is the element of PageTable
** Each element(or instance) records four values
** Size of PageElement is 10 bytes
**/
class PageElement
{
public:
    PageElement();

private:
    /**
    ** Four values in it, some problems
    ** File size less than 2**32?
    ** m_*size less than 2**8 = 256 bytes?
    **/
    uint32_t   m_hashVal, m_datPos;
    uint8_t    m_keySize, m_datSize;

private:
    friend class PageTable;
    friend class PageDB;
    friend ostream & operator << (ostream & os, PageElement & e);
};

/**
** PageTable means the page index of PageDB
** Size of PageTable is approx equal to 100*10 bytes
**/
class PageTable : public Noncopyable
{
public:
    PageTable(PageDB * db);

public:
    /**
    ** Need to be further optimizated
    ** Buffer -> PageContent
    **/
    BufferPacket getPacket();
    void         setByBucket(BufferPacket & packet);

private:
    /**
    ** Whether full
    **/
    bool   full();
    /**
    ** put into page
    **/
    bool   put(const Slice & key, const Slice & value, uint32_t hashVal, uint64_t offset);
    /**
    ** get from page
    **/
    Slice  get(const Slice & key, uint32_t hashVal);
    /**
    ** remove from page
    **/
    bool   remove(const Slice & key, uint32_t hashVal);
    /**
    ** find the index of approxate key
    **/
    bool   find(const Slice & key, uint32_t hashVal, int &index);
private:
    int m_d, m_curNum;
    PageElement m_elements[PAGESIZE + 5];

    PageDB * m_db;

    friend class PageDB;
};

class PageDB : public DBInternal
{
private:
    typedef pair<Slice, Slice> Node;

public:
    PageDB(HashFunc hashFunc = MurmurHash3);
    ~PageDB();

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
    bool     put(WriteBatch * pbatch);
    /**
    ** Layer 3
    **/
    void     sync();
    void     dump(ostream&os);
    void     compact();

private:
    /**
    ** Internal functions
    **/
    bool    write(WriteBatch * pbatch);
    bool    writeBatch(WriteBatch * pbatch);
    /**
    ** Release buffer for EmptyBlock
    **/
    void     recycle(int offset, int size);
    /**
    ** Update Index information into idxfile
    **/
    void     writeToIdxFile();
    void     readFromIdxFile();
    int      findSuitableOffset(int size);
    void     fullAddLocalD(int cur, uint64_t num, uint64_t pos1, uint64_t pos2, uint64_t od);
    void     readAndSetPage(PageTable *page, uint64_t addr);
    /**
    ** Assert 2**n
    **/
    bool     is2Exp(uint64_t val);
    
private:
    struct Writer;
    WriteBatch* buildBatchGroup(Writer ** last_writer);

    deque<Writer*> m_writers;
    WriteBatch *   m_tmpBatch;
    Mutex          m_writerlock;

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
    PageCache   *m_cache;

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
    /**
    ** Friend class
    **/
    friend class PageCache;
    friend class PageEmptyBlock;
    friend class PageTable;
};

/**
** PageCache is the archive of CacheElement
** Each CacheElement contains the pointer to the
** Page, the address and updated status.
**/
class PageCache : public Noncopyable
{
public:
    PageCache(PageDB * db);
    ~PageCache();

private:
    /**
    ** Free all cache_elements
    ** If updated is setted to true
    ** update it to file
    **/
    void        free();
    /**
    ** Find a Page In Cache
    **/
    PageTable * find(uint32_t addr, uint32_t & index);
    /**
    ** Put a Page into Cache
    **/
    int         put(PageTable * page, int addr);
    /**
    ** Set updated flag for the index-th element
    **/
    void        updated(int index);
    /**
    ** Sync, behave as free
    **/
    void        sync();

private:
    void        reset(int index);

private:
    class CacheElem
    {
    public:
        PageTable *m_page;
        uint32_t   m_addr;
        bool       m_updated;

        CacheElem() : m_page(NULL)
        {
            reset();
        }

        void reset()
        {
            if(m_page != NULL) delete m_page;
            m_page    = NULL;
            m_addr   = -1;
            m_updated = false;
        }
    };


private:
    PageDB *  m_db;
    CacheElem m_eles[CACHESIZE];
    int       m_cur;
    friend class PageDB;
};

};
#endif
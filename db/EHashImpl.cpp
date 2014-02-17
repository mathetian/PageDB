#ifndef _Factory_IMPL_H
#define _Factory_IMPL_H

#include <vector>
#include <fstream>
#include <queue>
using namespace std;

#include <sys/stat.h>

#include "Factory.h"
#include "BufferPacket.h"
#include "HashFunction.h"
#include "Thread.h"
#include "TimeStamp.h"

namespace customdb
{

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
    {
    public:
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
    PageElement(): m_hashVal(0), m_datPos(-1), m_keySize(-1), m_datSize(-1) {}
    void  clear()
    {
        m_hashVal = -1;
        m_datPos = -1;
        m_keySize = -1;
        m_datSize = -1;
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
    Page(ExtendibleHash * eHash) : d(0), curNum(0)
    {
        this -> eHash = eHash;
        m_log = Log::GetInstance();
    }

public:
    BufferPacket getPacket();
    void         setByBucket(BufferPacket & packet);
    int          getSize()
    {
        return SINT*2 + sizeof(elements);
    }

private:
    bool   full()
    {
        if(curNum >= PAGESIZE + 3) printf("xxx:%d\n", curNum);
        assert(curNum < PAGESIZE + 3);
        return curNum >= PAGESIZE;
    }
    bool   put(const Slice & key,const Slice & value, uint32_t hashVal);
    Slice  get(const Slice & key, uint32_t hashVal);
    bool   remove(const Slice & key, uint32_t hashVal);
    void   replaceQ(const Slice & key, const Slice & value, uint32_t hashVal, int offset);

private:
    void   addElement(PageElement element)
    {
        elements[curNum++] = element;
    }

    PageElement getElement(int index)
    {

    }

    PageElement getElement2(int index)
    {
        return elements[index];
    }

    int    getCurNum() const
    {
        return curNum;
    }
    int    getD() const
    {
        return d;
    }
    void   setCurNum(int num)
    {
        curNum = num;
    }
    void   setD(int d)
    {
        this -> d = d;
    }

private:
    friend class ExtendibleHash;
    ExtendibleHash * eHash;
    friend class PageCache;

private:
    /**
       Asscoiate with the file,
       Use PAGESIZE + 5 to avoid some special(boundary) situation
    **/
    volatile int d, curNum;
    PageElement elements[PAGESIZE + 5];
    Log * m_log;
};

class ExtendibleHash : public Factory
{
public:
    ExtendibleHash(HASH hashFunc = MurmurHash3);
    virtual ~ExtendibleHash();

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

    void    fflush();

    /**To speed up the batch progress, we use replace instead of put(that means we don't check whether it will be successful)**/
    void    runBatch(const WriteBatch * pbatch);
    void    runBatch2(const WriteBatch * pbatch);

    void    compact();

private:
    void     recycle(int offset, int size);
    void     writeToIdxFile(); /**Initialization, so write something into file**/
    void     readFromFile();/**Read the Index information**/
    int      findSuitableOffset(int size);
    void     printThisPage(Page * page);

    void     fullAddLocalD(int cur, uint64_t num, uint64_t pos1, uint64_t pos2, uint64_t od);

private:
    HASH      hashFunc;
    bool      updated, eupdated; /**two files update status**/
    fstream   idxfs, datfs;
    PageCache * pcache;

private:
    friend class Page;
    friend class PageCache;

private:
    /**Need read from file**/
    volatile int  gd, pn, fb;
    vector <uint64_t>  entries; /**Page entries, just offset for each page**/
    struct Writer;
    const uint64_t MOD;

private:
    deque<Writer*> m_writers;
    WriteBatch *   m_tmpBatch;
    Mutex          m_mutex;
    WriteBatch *   BuildBatchGroup(Writer ** last_writer);

    string         idxName;
    string         datName;
    volatile       int notnotnot;
public:
    void           write(WriteBatch* pbatch);

private:
    /**If not use flush, idx data won't be flushed into file.**/
    Mutex          datLock;
    Mutex          cacheLock;
    RWLock         globalLock;
    Mutex          tmplock;
    Mutex          cacheElemLock[CACHESIZE];

private:
    int            globalTimes;
    TimeStamp      m_ts;
    struct timeval m_totalTime;
};

typedef struct _tCacheElem
{
    Page * page;
    uint32_t entry;
    bool   updated;
    _tCacheElem() : page(NULL)
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

} CacheElem;

/**
    Can use a background thread write the data into files !!! Todo list.
**/
class PageCache
{
public:
    PageCache(ExtendibleHash * eHash) : cur(0), eHash(eHash) { }
    ~PageCache()
    {
        free();
    }

    /**when we need use free, we must lock everything visable.**/
    void    free()
    {
        int i;
        for(i = 0; i < CACHESIZE; i++)
        {
            if(cacheElems[i].updated == true)
            {
                Page * page = cacheElems[i].page;
                assert(page != NULL);
                eHash -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
                BufferPacket packet = page -> getPacket();
                eHash -> datfs.write(packet.getData(),packet.getSize());
            }
            cacheElems[i].reset();
        }

        cur = 0;

        eHash -> datfs.flush();
    }

    void freeWithLock()
    {
        ScopeMutex scope(&(eHash -> cacheLock));

        int i;
        for(i = 0; i < CACHESIZE; i++)
        {
            ScopeMutex scope1(&(eHash -> cacheElemLock[i]));

            if(cacheElems[i].updated == true)
            {
                Page * page = cacheElems[i].page;
                assert(page != NULL);
                BufferPacket packet = page -> getPacket();
                {
                    ScopeMutex scope2(&(eHash -> datLock));
                    eHash -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
                    eHash -> datfs.write(packet.getData(),packet.getSize());
                }
            }
            cacheElems[i].reset();
        }
        cur = 0;

        {
            ScopeMutex scope(&(eHash -> datLock));
            eHash -> datfs.flush();
        }
    }

    /**
        As some need be proceed outside of this function
        Lock should be in EHash.
    **/
    Page * find(uint32_t addr, int & index)
    {
        int i;
        for(i = 0; i < CACHESIZE; i++)
        {
            if(cacheElems[i].entry == addr)
            {
                index = i;
                cur = (i+1)%CACHESIZE;
                return cacheElems[i].page;
            }
        }
        return NULL;
    }

    /**
        Same reason as PageCache.find
        However, need further process.
    **/
    int putInto(Page * page, int addr)
    {
        /**
            Hard to implement
        **/
        int i = (cur+1)%CACHESIZE;
        for(; i != cur; i = (i+1)%CACHESIZE)
        {
            if(cacheElems[i].updated == false)
            {
                cacheElems[i].reset();

                cacheElems[i].page  = page;
                cacheElems[i].entry = addr;
                break;
            }
        }

        int oldcur = i;

        if(i == cur)
        {
            if(cacheElems[i].updated == true)
                reset(i);

            cacheElems[i].reset();

            cacheElems[i].page = page;
            cacheElems[i].entry = addr;
        }

        cur = (i+1)%CACHESIZE;

        return oldcur;
    }

    int putIntoWithLock(Page * page, int addr)
    {
        int i = (cur+1)%CACHESIZE;
        for(; i != cur; i = (i+1)%CACHESIZE)
        {
            if(cacheElems[i].updated == false)
            {
                if(eHash -> cacheElemLock[i].trylock() == 0)
                {
                    if(cacheElems[i].updated == true)
                    {
                        resetWithDatLock(i);
                    }
                    cacheElems[i].reset();

                    cacheElems[i].page = page;
                    cacheElems[i].entry = addr;
                    break;
                }
            }

        }

        int oldcur = i;

        if(i == cur)
        {
            /**ScopeMutex scope(&(eHash -> cacheElemLock[i]));**/

            eHash -> cacheElemLock[i].lock();


            if(cacheElems[i].updated == true)
                resetWithDatLock(i);

            cacheElems[i].reset();

            cacheElems[i].page = page;
            cacheElems[i].entry = addr;
        }

        cur = (i+1)%CACHESIZE;
        /**Dirty code, only when it get the owner of this page, it can release the cachelock.**/

        // eHash -> cacheLock.unlock();

        return oldcur;
    }
    /**Don't need further lock**/
    void setUpdated(int index)
    {
        cacheElems[index].updated = true;
    }

    int  findLockable(Page * page, uint32_t addr)
    {
        int i = (cur+1)%CACHESIZE;
        for(; i!=cur; i=(i+1)%CACHESIZE)
        {
            if(eHash->cacheElemLock[i].trylock() == 0)
            {
                if(cacheElems[i].updated == true)
                {
                    resetWithDatLock(i);
                }
                cacheElems[i].reset();

                cacheElems[i].page = page;
                cacheElems[i].entry = addr;

                cur = (i+1)%CACHESIZE;
                return i;
            }
        }
        return -1;
    }
private:
    /**Just need require datlock**/
    void resetWithDatLock(int index)
    {
        Page * page1 = cacheElems[index].page;
        assert(page1 != NULL);
        BufferPacket packet = page1 -> getPacket();

        {
            ScopeMutex scope(&(eHash -> datLock));
            eHash -> m_ts.StartTime();
            eHash -> datfs.seekg(cacheElems[index].entry, ios_base::beg);
            eHash -> datfs.write(packet.getData(),packet.getSize());
            eHash -> m_ts.StopTime();
            eHash -> m_ts.AddTime(eHash -> m_totalTime);
        }
    }

    void reset(int index)
    {
        Page * page1 = cacheElems[index].page;
        assert(page1 != NULL);
        BufferPacket packet = page1 -> getPacket();

        eHash -> m_ts.StartTime();
        eHash -> datfs.seekg(cacheElems[index].entry, ios_base::beg);
        eHash -> datfs.write(packet.getData(),packet.getSize());
        eHash -> m_ts.StopTime();
        eHash -> m_ts.AddTime(eHash -> m_totalTime);
    }

public:
    void fflush()
    {
        for(int i = 0; i < CACHESIZE; i++)
        {
            if(cacheElems[i].updated == true)
            {
                Page * page = cacheElems[i].page;
                eHash -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
                BufferPacket packet = page -> getPacket();
                eHash -> datfs.write(packet.getData(),packet.getSize());
                cacheElems[i].updated = false;
            }
        }
        cur = 0;
    }

    void fflushWithLock()
    {
        ScopeMutex scope(&(eHash -> cacheLock));

        for(int i = 0; i < CACHESIZE; i++)
        {
            ScopeMutex scope1(&(eHash -> cacheElemLock[i]));
            if(cacheElems[i].updated == true)
            {
                Page * page = cacheElems[i].page;
                eHash -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
                BufferPacket packet = page -> getPacket();
                eHash -> datfs.write(packet.getData(),packet.getSize());
                cacheElems[i].updated = false;
            }
        }
        cur = 0;
    }

private:
    ExtendibleHash * eHash;
    CacheElem cacheElems[CACHESIZE];
    int cur;
    friend class ExtendibleHash;
};

};
#endif#ifndef _Factory_IMPL_H
#define _Factory_IMPL_H

#include <vector>
#include <fstream>
#include <queue>
using namespace std;

#include <sys/stat.h>

#include "Factory.h"
#include "BufferPacket.h"
#include "HashFunction.h"
#include "Thread.h"
#include "TimeStamp.h"

namespace customdb
{

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
    {
    public:
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
    PageElement(): m_hashVal(0), m_datPos(-1), m_keySize(-1), m_datSize(-1) {}
    void  clear()
    {
        m_hashVal = -1;
        m_datPos = -1;
        m_keySize = -1;
        m_datSize = -1;
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
    Page(ExtendibleHash * eHash) : d(0), curNum(0)
    {
        this -> eHash = eHash;
        m_log = Log::GetInstance();
    }

public:
    BufferPacket getPacket();
    void         setByBucket(BufferPacket & packet);
    int          getSize()
    {
        return SINT*2 + sizeof(elements);
    }

private:
    bool   full()
    {
        if(curNum >= PAGESIZE + 3) printf("xxx:%d\n", curNum);
        assert(curNum < PAGESIZE + 3);
        return curNum >= PAGESIZE;
    }
    bool   put(const Slice & key,const Slice & value, uint32_t hashVal);
    Slice  get(const Slice & key, uint32_t hashVal);
    bool   remove(const Slice & key, uint32_t hashVal);
    void   replaceQ(const Slice & key, const Slice & value, uint32_t hashVal, int offset);

private:
    void   addElement(PageElement element)
    {
        elements[curNum++] = element;
    }

    PageElement getElement(int index)
    {

    }

    PageElement getElement2(int index)
    {
        return elements[index];
    }

    int    getCurNum() const
    {
        return curNum;
    }
    int    getD() const
    {
        return d;
    }
    void   setCurNum(int num)
    {
        curNum = num;
    }
    void   setD(int d)
    {
        this -> d = d;
    }

private:
    friend class ExtendibleHash;
    ExtendibleHash * eHash;
    friend class PageCache;

private:
    /**
       Asscoiate with the file,
       Use PAGESIZE + 5 to avoid some special(boundary) situation
    **/
    volatile int d, curNum;
    PageElement elements[PAGESIZE + 5];
    Log * m_log;
};

class ExtendibleHash : public Factory
{
public:
    ExtendibleHash(HASH hashFunc = MurmurHash3);
    virtual ~ExtendibleHash();

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

    void    fflush();

    /**To speed up the batch progress, we use replace instead of put(that means we don't check whether it will be successful)**/
    void    runBatch(const WriteBatch * pbatch);
    void    runBatch2(const WriteBatch * pbatch);

    void    compact();

private:
    void     recycle(int offset, int size);
    void     writeToIdxFile(); /**Initialization, so write something into file**/
    void     readFromFile();/**Read the Index information**/
    int      findSuitableOffset(int size);
    void     printThisPage(Page * page);

    void     fullAddLocalD(int cur, uint64_t num, uint64_t pos1, uint64_t pos2, uint64_t od);

private:
    HASH      hashFunc;
    bool      updated, eupdated; /**two files update status**/
    fstream   idxfs, datfs;
    PageCache * pcache;

private:
    friend class Page;
    friend class PageCache;

private:
    /**Need read from file**/
    volatile int  gd, pn, fb;
    vector <uint64_t>  entries; /**Page entries, just offset for each page**/
    struct Writer;
    const uint64_t MOD;

private:
    deque<Writer*> m_writers;
    WriteBatch *   m_tmpBatch;
    Mutex          m_mutex;
    WriteBatch *   BuildBatchGroup(Writer ** last_writer);

    string         idxName;
    string         datName;
    volatile       int notnotnot;
public:
    void           write(WriteBatch* pbatch);

private:
    /**If not use flush, idx data won't be flushed into file.**/
    Mutex          datLock;
    Mutex          cacheLock;
    RWLock         globalLock;
    Mutex          tmplock;
    Mutex          cacheElemLock[CACHESIZE];

private:
    int            globalTimes;
    TimeStamp      m_ts;
    struct timeval m_totalTime;
};

typedef struct _tCacheElem
{
    Page * page;
    uint32_t entry;
    bool   updated;
    _tCacheElem() : page(NULL)
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

} CacheElem;

/**
    Can use a background thread write the data into files !!! Todo list.
**/
class PageCache
{
public:
    PageCache(ExtendibleHash * eHash) : cur(0), eHash(eHash) { }
    ~PageCache()
    {
        free();
    }

    /**when we need use free, we must lock everything visable.**/
    void    free()
    {
        int i;
        for(i = 0; i < CACHESIZE; i++)
        {
            if(cacheElems[i].updated == true)
            {
                Page * page = cacheElems[i].page;
                assert(page != NULL);
                eHash -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
                BufferPacket packet = page -> getPacket();
                eHash -> datfs.write(packet.getData(),packet.getSize());
            }
            cacheElems[i].reset();
        }

        cur = 0;

        eHash -> datfs.flush();
    }

    void freeWithLock()
    {
        ScopeMutex scope(&(eHash -> cacheLock));

        int i;
        for(i = 0; i < CACHESIZE; i++)
        {
            ScopeMutex scope1(&(eHash -> cacheElemLock[i]));

            if(cacheElems[i].updated == true)
            {
                Page * page = cacheElems[i].page;
                assert(page != NULL);
                BufferPacket packet = page -> getPacket();
                {
                    ScopeMutex scope2(&(eHash -> datLock));
                    eHash -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
                    eHash -> datfs.write(packet.getData(),packet.getSize());
                }
            }
            cacheElems[i].reset();
        }
        cur = 0;

        {
            ScopeMutex scope(&(eHash -> datLock));
            eHash -> datfs.flush();
        }
    }

    /**
        As some need be proceed outside of this function
        Lock should be in EHash.
    **/
    Page * find(uint32_t addr, int & index)
    {
        int i;
        for(i = 0; i < CACHESIZE; i++)
        {
            if(cacheElems[i].entry == addr)
            {
                index = i;
                cur = (i+1)%CACHESIZE;
                return cacheElems[i].page;
            }
        }
        return NULL;
    }

    /**
        Same reason as PageCache.find
        However, need further process.
    **/
    int putInto(Page * page, int addr)
    {
        /**
            Hard to implement
        **/
        int i = (cur+1)%CACHESIZE;
        for(; i != cur; i = (i+1)%CACHESIZE)
        {
            if(cacheElems[i].updated == false)
            {
                cacheElems[i].reset();

                cacheElems[i].page  = page;
                cacheElems[i].entry = addr;
                break;
            }
        }

        int oldcur = i;

        if(i == cur)
        {
            if(cacheElems[i].updated == true)
                reset(i);

            cacheElems[i].reset();

            cacheElems[i].page = page;
            cacheElems[i].entry = addr;
        }

        cur = (i+1)%CACHESIZE;

        return oldcur;
    }

    int putIntoWithLock(Page * page, int addr)
    {
        int i = (cur+1)%CACHESIZE;
        for(; i != cur; i = (i+1)%CACHESIZE)
        {
            if(cacheElems[i].updated == false)
            {
                if(eHash -> cacheElemLock[i].trylock() == 0)
                {
                    if(cacheElems[i].updated == true)
                    {
                        resetWithDatLock(i);
                    }
                    cacheElems[i].reset();

                    cacheElems[i].page = page;
                    cacheElems[i].entry = addr;
                    break;
                }
            }

        }

        int oldcur = i;

        if(i == cur)
        {
            /**ScopeMutex scope(&(eHash -> cacheElemLock[i]));**/

            eHash -> cacheElemLock[i].lock();


            if(cacheElems[i].updated == true)
                resetWithDatLock(i);

            cacheElems[i].reset();

            cacheElems[i].page = page;
            cacheElems[i].entry = addr;
        }

        cur = (i+1)%CACHESIZE;
        /**Dirty code, only when it get the owner of this page, it can release the cachelock.**/

        // eHash -> cacheLock.unlock();

        return oldcur;
    }
    /**Don't need further lock**/
    void setUpdated(int index)
    {
        cacheElems[index].updated = true;
    }

    int  findLockable(Page * page, uint32_t addr)
    {
        int i = (cur+1)%CACHESIZE;
        for(; i!=cur; i=(i+1)%CACHESIZE)
        {
            if(eHash->cacheElemLock[i].trylock() == 0)
            {
                if(cacheElems[i].updated == true)
                {
                    resetWithDatLock(i);
                }
                cacheElems[i].reset();

                cacheElems[i].page = page;
                cacheElems[i].entry = addr;

                cur = (i+1)%CACHESIZE;
                return i;
            }
        }
        return -1;
    }
private:
    /**Just need require datlock**/
    void resetWithDatLock(int index)
    {
        Page * page1 = cacheElems[index].page;
        assert(page1 != NULL);
        BufferPacket packet = page1 -> getPacket();

        {
            ScopeMutex scope(&(eHash -> datLock));
            eHash -> m_ts.StartTime();
            eHash -> datfs.seekg(cacheElems[index].entry, ios_base::beg);
            eHash -> datfs.write(packet.getData(),packet.getSize());
            eHash -> m_ts.StopTime();
            eHash -> m_ts.AddTime(eHash -> m_totalTime);
        }
    }

    void reset(int index)
    {
        Page * page1 = cacheElems[index].page;
        assert(page1 != NULL);
        BufferPacket packet = page1 -> getPacket();

        eHash -> m_ts.StartTime();
        eHash -> datfs.seekg(cacheElems[index].entry, ios_base::beg);
        eHash -> datfs.write(packet.getData(),packet.getSize());
        eHash -> m_ts.StopTime();
        eHash -> m_ts.AddTime(eHash -> m_totalTime);
    }

public:
    void fflush()
    {
        for(int i = 0; i < CACHESIZE; i++)
        {
            if(cacheElems[i].updated == true)
            {
                Page * page = cacheElems[i].page;
                eHash -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
                BufferPacket packet = page -> getPacket();
                eHash -> datfs.write(packet.getData(),packet.getSize());
                cacheElems[i].updated = false;
            }
        }
        cur = 0;
    }

    void fflushWithLock()
    {
        ScopeMutex scope(&(eHash -> cacheLock));

        for(int i = 0; i < CACHESIZE; i++)
        {
            ScopeMutex scope1(&(eHash -> cacheElemLock[i]));
            if(cacheElems[i].updated == true)
            {
                Page * page = cacheElems[i].page;
                eHash -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
                BufferPacket packet = page -> getPacket();
                eHash -> datfs.write(packet.getData(),packet.getSize());
                cacheElems[i].updated = false;
            }
        }
        cur = 0;
    }

private:
    ExtendibleHash * eHash;
    CacheElem cacheElems[CACHESIZE];
    int cur;
    friend class ExtendibleHash;
};

class PageCache
{
public:
    PageCache(PageDB * eHash) : cur(0), eHash(eHash) { }
    ~PageCache()
    {
        free();
    }

    void    free()
    {
        int i;
        for(i = 0; i < CACHESIZE; i++)
        {
            if(cacheElems[i].updated == true)
            {
                PageTable * page = cacheElems[i].page;
                assert(page != NULL);
                eHash -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
                BufferPacket packet = page -> getPacket();
                eHash -> datfs.write(packet.getData(),packet.getSize());
            }
            cacheElems[i].reset();
        }

        cur = 0;

        eHash -> datfs.flush();
    }

    void freeWithLock()
    {
        ScopeMutex scope(&(eHash -> cacheLock));

        int i;
        for(i = 0; i < CACHESIZE; i++)
        {
            ScopeMutex scope1(&(eHash -> cacheElemLock[i]));

            if(cacheElems[i].updated == true)
            {
                PageTable * page = cacheElems[i].page;
                assert(page != NULL);
                BufferPacket packet = page -> getPacket();
                {
                    ScopeMutex scope2(&(eHash -> datLock));
                    eHash -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
                    eHash -> datfs.write(packet.getData(),packet.getSize());
                }
            }
            cacheElems[i].reset();
        }
        cur = 0;

        {
            ScopeMutex scope(&(eHash -> datLock));
            eHash -> datfs.flush();
        }
    }

    /**
        As some need be proceed outside of this function
        Lock should be in EHash.
    **/
    PageTable * find(uint32_t addr, int & index)
    {
        int i;
        for(i = 0; i < CACHESIZE; i++)
        {
            if(cacheElems[i].entry == addr)
            {
                index = i;
                cur = (i+1)%CACHESIZE;
                return cacheElems[i].page;
            }
        }
        return NULL;
    }

    /**
        Same reason as PageCache.find
        However, need further process.
    **/
    int putInto(PageTable * page, int addr)
    {
        /**
            Hard to implement
        **/
        int i = (cur+1)%CACHESIZE;
        for(; i != cur; i = (i+1)%CACHESIZE)
        {
            if(cacheElems[i].updated == false)
            {
                cacheElems[i].reset();

                cacheElems[i].page  = page;
                cacheElems[i].entry = addr;
                break;
            }
        }

        int oldcur = i;

        if(i == cur)
        {
            if(cacheElems[i].updated == true)
                reset(i);

            cacheElems[i].reset();

            cacheElems[i].page = page;
            cacheElems[i].entry = addr;
        }

        cur = (i+1)%CACHESIZE;

        return oldcur;
    }

    int putIntoWithLock(PageTable * page, int addr)
    {
        int i = (cur+1)%CACHESIZE;
        for(; i != cur; i = (i+1)%CACHESIZE)
        {
            if(cacheElems[i].updated == false)
            {
                if(eHash -> cacheElemLock[i].trylock() == 0)
                {
                    if(cacheElems[i].updated == true)
                    {
                        resetWithDatLock(i);
                    }
                    cacheElems[i].reset();

                    cacheElems[i].page = page;
                    cacheElems[i].entry = addr;
                    break;
                }
            }

        }

        int oldcur = i;

        if(i == cur)
        {
            /**ScopeMutex scope(&(eHash -> cacheElemLock[i]));**/

            eHash -> cacheElemLock[i].lock();


            if(cacheElems[i].updated == true)
                resetWithDatLock(i);

            cacheElems[i].reset();

            cacheElems[i].page = page;
            cacheElems[i].entry = addr;
        }

        cur = (i+1)%CACHESIZE;
        /**Dirty code, only when it get the owner of this page, it can release the cachelock.**/

        // eHash -> cacheLock.unlock();

        return oldcur;
    }
    /**Don't need further lock**/
    void setUpdated(int index)
    {
        cacheElems[index].updated = true;
    }

    int  findLockable(PageTable * page, uint32_t addr)
    {
        int i = (cur+1)%CACHESIZE;
        for(; i!=cur; i=(i+1)%CACHESIZE)
        {
            if(eHash->cacheElemLock[i].trylock() == 0)
            {
                if(cacheElems[i].updated == true)
                {
                    resetWithDatLock(i);
                }
                cacheElems[i].reset();

                cacheElems[i].page = page;
                cacheElems[i].entry = addr;

                cur = (i+1)%CACHESIZE;
                return i;
            }
        }
        return -1;
    }
private:
    /**Just need require datlock**/
    void resetWithDatLock(int index)
    {
        PageTable * page1 = cacheElems[index].page;
        assert(page1 != NULL);
        BufferPacket packet = page1 -> getPacket();

        {
            ScopeMutex scope(&(eHash -> datLock));
            eHash -> m_ts.StartTime();
            eHash -> datfs.seekg(cacheElems[index].entry, ios_base::beg);
            eHash -> datfs.write(packet.getData(),packet.getSize());
            eHash -> m_ts.StopTime();
            eHash -> m_ts.AddTime(eHash -> m_totalTime);
        }
    }

    void reset(int index)
    {
        PageTable * page1 = cacheElems[index].page;
        assert(page1 != NULL);
        BufferPacket packet = page1 -> getPacket();

        eHash -> m_ts.StartTime();
        eHash -> datfs.seekg(cacheElems[index].entry, ios_base::beg);
        eHash -> datfs.write(packet.getData(),packet.getSize());
        eHash -> m_ts.StopTime();
        eHash -> m_ts.AddTime(eHash -> m_totalTime);
    }

public:
    void fflush()
    {
        for(int i = 0; i < CACHESIZE; i++)
        {
            if(cacheElems[i].updated == true)
            {
                PageTable * page = cacheElems[i].page;
                eHash -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
                BufferPacket packet = page -> getPacket();
                eHash -> datfs.write(packet.getData(),packet.getSize());
                cacheElems[i].updated = false;
            }
        }
        cur = 0;
    }

    void fflushWithLock()
    {
        ScopeMutex scope(&(eHash -> cacheLock));

        for(int i = 0; i < CACHESIZE; i++)
        {
            ScopeMutex scope1(&(eHash -> cacheElemLock[i]));
            if(cacheElems[i].updated == true)
            {
                PageTable * page = cacheElems[i].page;
                eHash -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
                BufferPacket packet = page -> getPacket();
                eHash -> datfs.write(packet.getData(),packet.getSize());
                cacheElems[i].updated = false;
            }
        }
        cur = 0;
    }

private:
    PageDB * eHash;
    CacheElem cacheElems[CACHESIZE];
    int cur;
    friend class ExtendibleHash;
};

};
#endif
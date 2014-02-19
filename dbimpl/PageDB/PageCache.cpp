#include "PageDBImpl.h"

namespace customdb
{

PageCache::PageCache(PageDB * db) : \
    cur(0), db(db)
{

}

PageCache::~PageCache()
{
    free();
}

void    PageCache::free()
{
    int i;
    for(i = 0; i < CACHESIZE; i++)
    {
        if(cacheElems[i].updated == true)
        {
            PageTable * page = cacheElems[i].page;
            assert(page);
            db -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
            BufferPacket packet = page -> getPacket();
            db -> datfs.write(packet.getData(),packet.getSize());
        }
        cacheElems[i].reset();
    }

    cur = 0;

    db -> datfs.flush();
}

void    PageCache::freeWithLock()
{
    ScopeMutex scope(&(db -> cacheLock));

    int i;
    for(i = 0; i < CACHESIZE; i++)
    {
        ScopeMutex scope1(&(db -> cacheElemLock[i]));

        if(cacheElems[i].updated == true)
        {
            PageTable * page = cacheElems[i].page;
            assert(page);
            BufferPacket packet = page -> getPacket();
            {
                ScopeMutex scope2(&(db -> datLock));
                db -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
                db -> datfs.write(packet.getData(),packet.getSize());
            }
        }
        cacheElems[i].reset();
    }
    cur = 0;

    {
        ScopeMutex scope(&(db -> datLock));
        db -> datfs.flush();
    }
}


PageTable * PageCache::find(uint32_t addr, uint32_t & index)
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


int PageCache::putInto(PageTable * page, int addr)
{
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

void   PageCache::setUpdated(int index)
{
    cacheElems[index].updated = true;
}

int    PageCache::findLockable(PageTable * page, uint32_t addr)
{
    int i = (cur+1)%CACHESIZE;
    for(; i!=cur; i=(i+1)%CACHESIZE)
    {
        if(db -> cacheElemLock[i].trylock() == 0)
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

void   PageCache::resetWithDatLock(int index)
{
    PageTable * page1 = cacheElems[index].page;
    assert(page1);

    BufferPacket packet = page1 -> getPacket();

    {
        ScopeMutex scope(&(db -> datLock));
        db -> datfs.seekg(cacheElems[index].entry, ios_base::beg);
        db -> datfs.write(packet.getData(),packet.getSize());
    }
}

void   PageCache::reset(int index)
{
    PageTable * page1 = cacheElems[index].page;
    assert(page1 != NULL);
    BufferPacket packet = page1 -> getPacket();

    db -> datfs.seekg(cacheElems[index].entry, ios_base::beg);
    db -> datfs.write(packet.getData(),packet.getSize());
}


void   PageCache::fflush()
{
    for(int i = 0; i < CACHESIZE; i++)
    {
        if(cacheElems[i].updated == true)
        {
            PageTable * page = cacheElems[i].page;
            db -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
            BufferPacket packet = page -> getPacket();
            db -> datfs.write(packet.getData(),packet.getSize());
            cacheElems[i].updated = false;
        }
    }
    cur = 0;
}

void   PageCache::fflushWithLock()
{
    ScopeMutex scope(&(db -> cacheLock));

    for(int i = 0; i < CACHESIZE; i++)
    {
        ScopeMutex scope1(&(db -> cacheElemLock[i]));
        if(cacheElems[i].updated == true)
        {
            PageTable * page = cacheElems[i].page;
            db -> datfs.seekg(cacheElems[i].entry, ios_base::beg);
            BufferPacket packet = page -> getPacket();
            db -> datfs.write(packet.getData(),packet.getSize());
            cacheElems[i].updated = false;
        }
    }
    cur = 0;
}

};


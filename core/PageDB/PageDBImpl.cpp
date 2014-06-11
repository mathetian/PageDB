// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "PageDBImpl.h"

namespace customdb
{

PageDB::PageDB(HashFunc hashFunc) : m_HashFunc(hashFunc),  gd(0), pn(1), fb(-1)
{
    pcache = new PageCache(this);
}

PageDB::~PageDB()
{
    close();
}

/**
** Layer 1
**/
bool     PageDB::open(const string &filename)
{
    m_prefix = filename;

    string idxName = filename + ".idx";
    string datName = filename + ".dat";

    m_idxfile.Open(idxName);
    m_datfile.Open(datName);

    /**
    ** DB File Exist? Judge it before pre-processing
    **/
    if(FileModule::Size(idxName) == 0)
    {
        assert(FileModule::Size(datName) == 0);

        m_gd = 0; m_pn = 1; m_fb = -1;
        entries.push_back(0);
        writeToIdxFile();

        PageTable * page = new PageTable(this);
        BufferPacket packet = page -> getPacket();
        
        m_datfile.Append(packet.c_str(), SPAGETABLE);
        
        m_cache -> putInto(page, 0);
    }
    else
    {
        /**
        ** Exist old DB, read index information from file
        **/
        readFromFile();
    } 

    /**
    ** If no empty block, allocate one from the end of file
    **/
    if(m_fb == -1)
    {
        m_fb = m_datfile.Size();

        PageEmptyBlock block;

        block.eles[0].pos  = fb + SEEBLOCK;
        block.eles[0].size = SEEBLOCK;
        block.curNum       = 1;

        m_datfile.Append((char*)&block, SEEBLOCK);
        m_datfile.Append((char*)&block, SEEBLOCK);
    }

    m_log -> _Trace("DB Open Successfully");

    return true;
}

/**
** Close DB: Write buffer into files and delete cache buffer
**/
bool     PageDB::close()
{
    sync();

    m_datfile.Close();
    m_idxfile.Close();

    if(m_cache) delete m_cache;
    m_cache = NULL;

    return true;
}

bool     PageDB::put(const Slice & key,const Slice & value)
{
    uint32_t hashVal = m_HashFunc(key);
    uint32_t cur     = hashVal & ((1 << gd) -1);

    uint32_t index   = 0;
    uint64_t addr    =  m_entries.at(cur) & MOD;
    uint64_t digNum  = (m_entries.at(cur) & (~MOD));
    uint64_t pageNum = (digNum >> 56);

    PageTable * page = m_cache -> find(addr, index);
    bool entriesUpdate = false;

    if(page == NULL)
    {
        page = new PageTable(this);
        index = m_cache -> putInto(page, addr);
        
        BufferPacket packet(SPAGETABLE);
        
        m_datfile.Read(packet.str(), addr, SPAGETABLE);
        m_datfile.setCurSize(SPAGETABLE);

        page -> setByBucket(packet);
        
        assert(page -> curNum < PAGESIZE + 5);
    }

    if(page -> full() && page -> d == gd)
    {
        m_log -> _Trace("ExtendibleHash :: put :: full, so double-entries\n");

        gd += 1;

        uint32_t oldSize = m_entries.size();
        
        for(uint32_t i = 0; i < oldSize; i++)
        {
            uint64_t addr    = m_entries.at(i) &   MOD;
            uint64_t pageNum = m_entries.at(i) & (~MOD);
            pageNum >>= 56;
            pageNum++;
            pageNum <<= 56;
            m_entries[i] = addr | pageNum;
        }

        for(uint32_t i = 0; i < oldSize; i++)
            m_entries.push_back(m_entries.at(i));

        pageNum++;
    }

    if(page -> full() && page -> d < gd)
    {
        log -> _Trace("ExtendibleHash :: put :: split \n");

        if((page -> put(key, value, hashVal)) == 1)
        {
            pcache -> setUpdated(index);
        }
        else
        {
            log -> _Warn("ExtendibleHash :: put :: HAS been in the file\n");
            return false;
        }

        PageTable * p2 = new PageTable(this);

        int index = 0, curNum2 = 0, curNum3 = 0;

        for(index = 0; index < page -> curNum; index++)
        {
            PageElement element = page -> elements[index];

            int id = element.m_hashVal;

            int flag = (id & ((1 << gd) - 1));

            if(((flag >> (page -> d)) & 1) == 1)
                p2 -> elements[curNum3++] = page -> elements[index];
            else
                page -> elements[curNum2++] = page -> elements[index];
        }

        page -> curNum = curNum2;
        p2   -> curNum = curNum3;

        uint64_t oldpos  = addr;
        uint64_t oldpos2 = m_datfile.size();

        assert(pageNum - 1 >= 0);
        pageNum--;
        digNum = pageNum << 56;

        oldpos2 = oldpos2 | digNum;
        oldpos  = oldpos  | digNum;

        uint64_t pageNum2 = 1ull << (pageNum + 1);
        fullAddLocalD(cur, pageNum2, oldpos, oldpos2, page -> d);

        page -> d = p2 -> d = (page -> d) + 1;

        BufferPacket packe2 = p2 -> getPacket();
        m_datfile.Write(packe2.c_str(), -1, packe2.size());

        delete p2;
        p2 = NULL;

        pn += 1;
    }
    else
    {
        if((page -> put(key, value, hashVal)) == 1)
        {
            m_cache -> setUpdated(index);
        }
        else
        {
            log -> _Warn("ExtendibleHash :: put :: HAS been in the file\n");
            return false;
        }
    }

    writeToIdxFile();

    return true;
}

Slice    PageDB::get(const Slice & key)
{
    uint32_t hashVal = hashFunc(key);
    uint32_t cur = hashVal & ((1 << gd) -1);

    uint32_t index = 0;

    uint64_t addr    =  entries.at(cur) & MOD;

    PageTable * page = pcache -> find(addr, index);

    if(page == NULL)
    {
        page = new PageTable(this);
        pcache -> putInto(page, addr);

        BufferPacket packet(SPAGETABLE);

        m_datfile.Read(packet.str(), addr, packet.size());
        page -> setByBucket(packet);
    }

    Slice rs = page -> get(key, hashVal);

    return rs;
}

bool     PageDB::remove(const Slice & key)
{
    uint32_t hashVal = hashFunc(key);
    uint32_t cur     = hashVal & ((1 << gd) -1);

    uint32_t index = 0;

    uint64_t addr    =  entries.at(cur) & MOD;

    PageTable * page = pcache -> find(addr, index);

    if(page == NULL)
    {
        page = new PageTable(this);
        index = pcache -> putInto(page, addr);

        BufferPacket packet(SPAGETABLE);

        m_datfile.Read(packet.str(), addr, packet.size());
        page -> setByBucket(packet);
    }

    int rb = page -> remove(key, hashVal);

    if(rb == 1)
    {
        BufferPacket npacket = page -> getPacket();

        m_datfile.Read(npacket.str(), addr, npacket.size());
        pcache -> setUpdated(index);
    }
    else
        log -> _Warn("ExtendibleHash :: remove failed(maybe not exist?)");

    return rb;
}

/**
** Layer 2
**/
void     PageDB::put(const WriteBatch * pbatch)
{
    uint32_t curpos = m_datfile.size();

    BufferPacket phyPacket(pbatch->getTotalsize());
    m_datfile.Write(phyPacket.c_str(), -1, phyPacket.size());

    uint32_t totalSize = 0;
    WriteBatch::Iterator iterator(pbatch);

    for(const Node * node = iterator.first(); node != iterator.end(); node = iterator.next())
    {
        Slice key   = node -> first;
        Slice value = node -> second;

        uint32_t hashVal = hashFunc(key);
        uint32_t cur   = hashVal & ((1 << gd) -1);
        uint32_t index = 0;

        uint64_t addr    =  entries.at(cur) & MOD;
        uint64_t digNum  = (entries.at(cur) & (~MOD));
        uint64_t pageNum = (digNum >> 56);

        PageTable * page = pcache -> find(addr, index);

        if(page == NULL)
        {
            page  = new PageTable(this);
            index = pcache -> putInto(page, addr);

            BufferPacket packet(SPAGETABLE);

            m_datfile.Read(packet.str(), addr, packet.size());
            page -> setByBucket(packet);
        }

        if(page -> full() && page -> d == gd)
        {
            gd++;

            int oldSize = entries.size();
            for(int i = 0; i < oldSize; i++)
            {
                uint64_t addr1    = entries.at(i) & MOD;
                uint64_t pageNum2 = entries.at(i) & (~MOD);
                pageNum2 >>= 56;
                pageNum2++;
                pageNum2 <<= 56;
                entries[i] = addr1 | pageNum2;
            }

            for(int i = 0; i < oldSize; i++)
                entries.push_back(entries.at(i));

            pageNum++;
        }

        if(page -> full() && page -> d < gd)
        {
            page -> replaceQ(key, value, hashVal, totalSize + curpos);
            phyPacket << key << value;
            totalSize += key.size() + value.size();

            pcache -> setUpdated(index);

            PageTable * p2 = new PageTable(this);

            int index = 0, curNum2 = 0, curNum3 = 0;

            for(index = 0; index < page -> curNum; index++)
            {
                PageElement element = page -> elements[index];

                int id = element.m_hashVal;

                int flag = (id & ((1 << gd) - 1));

                if(((flag >> (page -> d)) & 1) == 1)
                    p2 -> elements[curNum3++] = page -> elements[index];
                else
                    page -> elements[curNum2++] = page -> elements[index];
            }

            page -> curNum = curNum2;
            p2   -> curNum = curNum3;

            uint64_t oldpos = addr;
            uint64_t oldpos2 = m_datfile.size();

            assert(pageNum - 1 >= 0);
            pageNum--;
            digNum = pageNum << 56;

            oldpos2 = oldpos2 | digNum;
            oldpos  = oldpos  | digNum;

            uint64_t pageNum2 = 1ull << (pageNum + 1);
            fullAddLocalD(cur, pageNum2, oldpos, oldpos2, page -> d);

            page -> d = p2 -> d = (page -> d) + 1;

            BufferPacket packe2 = p2 -> getPacket();
            m_datfile.Write(packe2.c_str(), -1, packe2.size());

            pn += 1;
            delete p2;
            p2 = NULL;
        }
        else
        {
            page -> replaceQ(key, value, hashVal, totalSize + curpos);
            phyPacket << key << value;
            totalSize = totalSize + key.size() + value.size();

            pcache -> setUpdated(index);
        }
    }

    m_datfile.Write(phyPacket.c_str(), curpos, phyPacket.size());
    writeToIdxFile();
    pcache -> free();
}

void     PageDB::write(WriteBatch* pbatch)
{
    if(pbatch == NULL) return;

    Writer w(&m_mutex);
    w.batch = pbatch;
    w.sync = false;
    w.done = false;

    ScopeMutex l(&m_mutex);
    m_writers.push_back(&w);
    while (!w.done && &w != m_writers.front())
    {
        w.cv.wait();
    }
    if (w.done)
    {
        return;
    }

    Writer* last_writer = &w;
    WriteBatch* updates = BuildBatchGroup(&last_writer);

    {
        m_mutex.unlock();
        /**
           When it is directed to here,
           only one thread can access it at same time.
        **/
        runBatch(updates);
        m_mutex.lock();
    }

    if (updates == m_tmpBatch) m_tmpBatch->clear();

    while (true)
    {
        Writer* ready = m_writers.front();
        m_writers.pop_front();

        if (ready != &w)
        {
            ready->done = true;
            ready->cv.signal();
        }

        if (ready == last_writer) break;
    }

    if (!m_writers.empty())
    {
        m_writers.front()->cv.signal();
    }
}

void    PageDB::runBatchParallel(const WriteBatch * pbatch)
{
    BufferPacket phyPacket(pbatch->getTotalsize());

    uint32_t curpos = 0, totalSize = 0;
    PageTable * page;

    {
        ScopeMutex scope(&datLock);

        curpos = m_datfile.size();
        m_datfile.Write(phyPacket.c_str(), -1, phyPacket.size());
    }

    WriteBatch::Iterator iterator(pbatch);

    for(const Node * node = iterator.first(); node != iterator.end(); node = iterator.next())
    {
        Slice key   = node -> first;
        Slice value = node -> second;

        uint32_t hashVal = hashFunc(key);
        /**Sooorry, I need use jump to**/
        int globalFlag = 0;
    LABLE:
        if(globalFlag == 0)
        {
            globalLock.readLock();
        }
        else if(globalFlag == 1)
        {
            printf("Need Split the Index\n");
            globalLock.writeLock();
        }

        uint32_t cur   = hashVal & ((1 << gd) -1);
        assert((1<<gd) == entries.size());
        uint32_t index = 0;

        uint64_t addr    =  entries.at(cur) & MOD;
        uint64_t digNum  = (entries.at(cur) & (~MOD));
        uint64_t pageNum = (digNum >> 56);

        page = NULL;

        {
            cacheLock.lock();
            assert(cur < entries.size());

            page = pcache -> find(addr, index);

            if(page != NULL)
            {
                if(cacheElemLock[index].trylock()==0)
                {
                    cacheLock.unlock();
                }
                else
                {
                    assert(globalFlag == 0);
                    cacheLock.unlock();
                    cacheElemLock[index].lock();
                }
            }
            else
                cacheLock.unlock();
        }

        if(page != NULL)
        {
            if(page == pcache -> cacheElems[index].page && addr == pcache -> cacheElems[index].entry)
            {
            }
            else
            {
                /**which situation will lead to different output**/
                /**Page different can be understand. However, entry?**/
                if(page != pcache -> cacheElems[index].page)
                    printf("notice21\n");
                else
                {
                    cout<<addr<<endl;
                    cout<<pcache -> cacheElems[index].entry<<endl;
                    printf("notice22\n");
                }

                cacheElemLock[index].unlock();
                assert(globalFlag == 0);
                globalLock.readUnlock();

                goto LABLE;
            }
        }
        else if(page == NULL)
        {
            bool flag1 = true;
            page  = new PageTable(this);

            index = -1;
            while(flag1)
            {
                {
                    ScopeMutex lock(&cacheLock);
                    index = pcache -> findLockable(page, addr);
                }

                if(globalFlag == 1) assert(index != -1);

                if(index != -1)
                {
                    flag1 = false;
                }
                else
                    printf("notice4\n");
            }

            BufferPacket packet(SPAGETABLE);

            {
                ScopeMutex lock(&datLock);
                assert(cur < entries.size());

                m_datfile.Read(packet.str(), addr, packet.size());
            }

            page -> setByBucket(packet);
        }

        int index2 = index;
        /**could use further optimization**/
        if(page -> full() && page -> getD() == gd)
        {
            if(globalFlag == 0)
            {
                globalLock.readUnlock();
                cacheElemLock[index].unlock();
                globalFlag = 1;
                goto LABLE;
            }

            gd++;

            int oldSize = entries.size();
            for(int i = 0; i < oldSize; i++)
            {
                uint64_t addr1    = entries.at(i) & MOD;
                uint64_t pageNum2 = entries.at(i) & (~MOD);
                pageNum2 >>= 56;
                pageNum2++;
                pageNum2 <<= 56;
                entries[i] = addr1 | pageNum2;
            }

            for(int i = 0; i < oldSize; i++)
                entries.push_back(entries.at(i));

            pageNum++;

            globalLock.writeUnlock();
            cacheElemLock[index].unlock();
            globalFlag = 0;

            goto LABLE;
        }
        else if(page -> full())
        {
            page -> replaceQ(key, value, hashVal, totalSize + curpos);
            phyPacket << key << value;
            totalSize += key.size() + value.size();

            pcache -> setUpdated(index2);

            PageTable * p2 = new PageTable(this);

            int index = 0, curNum2 = 0, curNum3 = 0;

            int ocurNum = page -> getCurNum();
            page -> setCurNum(0);

            for(index = 0; index < ocurNum; index++)
            {
                PageElement element = page -> elements[index];

                int id = element.m_hashVal;

                int flag = (id & ((1 << gd) - 1));

                if(((flag >> (page -> d)) & 1) == 1)
                    p2   -> addElement(page -> getElement2(index));
                else
                    page -> addElement(page -> getElement2(index));
            }

            uint64_t oldpos2;

            int od = page -> getD();

            p2   -> setD(page->getD() + 1);
            page -> setD(p2->getD());

            BufferPacket packe2 = p2 -> getPacket();

            {
                ScopeMutex scope(&datLock);
                oldpos2 = m_datfile.size();
                m_datfile.Append(packe2.c_str(), packe2.size());
            }

            uint64_t oldpos = addr;

            assert(pageNum - 1 >= 0);
            pageNum--;
            digNum = pageNum << 56;

            oldpos2 = oldpos2 | digNum;
            oldpos  = oldpos  | digNum;

            uint64_t pageNum2 = 1ull << (pageNum + 1);
            fullAddLocalD(cur, pageNum2, oldpos, oldpos2, od);

            delete p2;
            p2 = NULL;

            pn+=1;
        }
        else
        {
            page -> replaceQ(key, value, hashVal, totalSize + curpos);

            phyPacket << key << value;
            totalSize += key.size() + value.size();

            pcache -> setUpdated(index);
        }

        if(globalFlag == 0)
        {
            globalLock.readUnlock();
        }
        else
        {
            globalLock.writeUnlock();
        }

        cacheElemLock[index2].unlock();

    }

    {
        ScopeMutex scope(&datLock);
        m_datfile.Write(phyPacket.c_str(), curpos, phyPacket.size());
    }
}

/**
** Layer 3
**/
void    PageDB::sync()
{
    pcache -> free();
}

void     PageDB::dump()
{
    pcache -> fflush();

    PageTable * page = new PageTable(this);
    cout << gd << " " << pn << " " << entries.size() << endl;
    for(int cur = 0, index = 0; cur < entries.size(); cur++)
    {
        int j;
        for(j = 0; j < cur; j++)
        {
            if(entries.at(j) ==  entries.at(cur)) break;
        }

        if(j != cur) continue;

        uint64_t addr    = entries.at(cur) & MOD;

        BufferPacket packet(SPAGETABLE);

        m_datfile.Read(packet.str(), addr, packet.size());
        page -> setByBucket(packet);

        cout << "Page(size:"<<page -> curNum <<") " << index++ <<" "<<cur <<" "<<page -> d<<":";

        for(j = 0; j < page -> curNum; j++)
        {
            BufferPacket packet(2*SINT);
            m_datfile.Read(packet.str(), page -> elements[j].m_datPos, packet.size());
            int a, b;
            uint32_t hashVal = page -> elements[j].m_hashVal;
            packet >> a >> b;
            cout << a <<" " << b << " "<<hashVal<<" ";
        }

        cout << endl;
    }

    delete page;
    page = NULL;
}

void    PageDB::compact()
{
    fflush();

    PageTable * page = new PageTable(this);

    RandomFile tmpfile1, tmpfile2;
    tmpfile1.Open("tmppage.bak");
    tmpfile2.Open("tmpcon.bak");
    tmpfile1.Truncate(0);
    tmpfile2.Truncate(0);

    WriteBatch * pbatch =  new WriteBatch(PAGESIZE*2);

    for(int cur = 0; cur < entries.size(); cur++)
    {
        int j;
        for(j = 0; j < cur; j++)
        {
            if(entries.at(j) ==  entries.at(cur)) break;
        }

        if(j != cur) continue;

        uint64_t addr    = entries.at(cur) & MOD;

        BufferPacket packet(SPAGETABLE);

        m_datfile.Read(packet.str(), addr, packet.size());
        page -> setByBucket(packet);

        for(j = 0; j < page -> curNum; j++)
        {
            PageElement element = page -> elements[j];

            BufferPacket packet1(element.m_keySize + element.m_datSize);
            m_datfile.Read(packet1.str(), element.m_datPos, packet1.size());
            Slice a(element.m_keySize), b(element.m_datSize);
            packet1 >> a >> b;
            pbatch -> put(a,b);
        }

        tmpfile1.Append(packet.c_str(), packet.size());

        BufferPacket packet1(WriteBatchInternal::ByteSize(pbatch));
        WriteBatch::Iterator iter(pbatch);
        for(const Node * node = iter.first(); node != iter.end(); node = iter.next())
        {
            packet1 << (node -> first) << (node -> second);
        }
        uint32_t size = packet1.size();

        tmpfile2.Append((char*)&(size), sizeof(uint32_t));
        tmpfile2.Append(packet1.c_str(), packet1.size());

        pbatch -> clear();
    }

    m_datfile.Truncate(0);
    m_idxfile.Truncate(0);

    vector<char> used(entries.size(), 0);

    uint32_t uds = 0;
    int pos1 = 0, pos2 = 0;
    for(int cur = 0; cur < entries.size(); cur++)
    {
        if(used.at(cur) == 0)
        {
            vector<int> ids;
            ids.push_back(cur);

            for(int j = cur + 1; j < entries.size(); j++)
            {
                if(entries.at(cur) == entries.at(j))
                {
                    used[j] = 1;
                    ids.push_back(j);
                }
            }

            if(ids.size() != 1)
                assert(ids.size() % 2 == 0);

            for(int j=0; j<ids.size(); j++)
            {
                uint64_t pageNum = ids.size();
                uint64_t k = 0;
                while(pageNum > 1)
                {
                    k++;
                    pageNum >>= 1;
                }
                entries[ids.at(j)] = uds | (k << 56);
            }

            BufferPacket packet(SPAGETABLE);
            tmpfile1.Read(packet.str(), pos1, packet.size());
            pos1 += packet.size();

            page -> setByBucket(packet);

            uint32_t size;
            tmpfile2.Read((char*)&size, pos2, sizeof(uint32_t));
            pos2 += sizeof(uint32_t);

            BufferPacket packet1(size);
            tmpfile2.Read(packet1.str(), pos2, packet1.size());
            pos2 += packet1.size();

            int fpos = uds + packet.size();
            for(int j=0; j<page->curNum; j++)
            {
                page->elements[j].m_datPos = fpos;
                fpos += page->elements[j].m_keySize + page->elements[j].m_datSize;
            }

            packet = page->getPacket();
            m_datfile.Write(packet.c_str(), -1, packet.size());
            m_datfile.Write(packet1.c_str(), -1, packet1.size());

            uds += packet.size() + packet1.size();
        }
    }

    this -> fb = -1;
    writeToIdxFile();

    delete page;
    page = NULL;
    delete pbatch;
    pbatch = NULL;

    tmpfile1.Truncate(0);
    tmpfile2.Truncate(0);
}

/**
** Internal functions
**/    
void    PageDB::recycle(int offset, int size)
{
    ScopeMutex scope(&(datLock));

    PageEmptyBlock block;

    if(fb == -1)
    {
        log -> _Fatal("There must be some fatal error\n");
        return;
    }

    m_datfile.Read((char*)&block, fb, SEEBLOCK);
    if(block.curNum == PAGESIZE)
    {
        int nn = block.nextBlock;

        PageEmptyBlock nnBlock = block.split();
        nnBlock.nextBlock   = nn;
        block.nextBlock     = m_datfile.size();

        m_datfile.Write((char*)&nnBlock, -1, SEEBLOCK);
    }

    block.eles[block.curNum].pos    = offset;
    block.eles[block.curNum++].size = size;

    m_datfile.Write((char*)&block, fb, SEEBLOCK);
}

void     PageDB::writeToIdxFile()
{
    BufferPacket packet(SINT * 3 + entries.size()*SINT64);
    packet << gd << pn << fb;
    packet.write((char*)&entries[0], entries.size()*SINT64);
    m_idxfile.Write(packet.c_str(), 0, packet.size());
}

void     PageDB::readFromFile()
{
    BufferPacket packet(SINT * 3);
    m_idxfile.Read(packet.str(), 0, packet.size());

    int gd1, pn1, fb1;
    packet >> gd1 >> pn1 >> fb1;
    gd = gd1;
    pn = pn1;
    fb = fb1;

    entries = vector<uint64_t> (1 << gd, 0);

    m_idxfile.Read((char*)&entries[0], SINT * 3, entries.size()*SINT64);
}

int      PageDB::findSuitableOffset(int size)
{
    PageEmptyBlock block;
    int offset, pos;
    assert(fb != -1);
    m_datfile.Read((char*)&block, fb, SEEBLOCK);
    if(block.nextBlock != -1)
    {
        if(block.curNum < PAGESIZE/2)
        {
            PageEmptyBlock nnBlock;

            int old = block.nextBlock;

            m_datfile.Write((char*)&nnBlock, block.nextBlock, SEEBLOCK);

            int index = 0;
            int bnum = block.curNum;

            block.nextBlock = nnBlock.nextBlock;

            for(; index < nnBlock.curNum; index++)
            {
                block.eles[block.curNum++] = nnBlock.eles[index];
                if(block.curNum == PAGESIZE)
                {
                    PageEmptyBlock nnn = block.split();
                    nnn.nextBlock   = block.nextBlock;
                    block.nextBlock = m_datfile.size();

                    m_datfile.Write((char*)&nnn,-1, SEEBLOCK);
                }
            }

            if(block.curNum == PAGESIZE)
            {
                PageEmptyBlock nnn = block.split();
                nnn.nextBlock   = block.nextBlock;
                block.nextBlock = m_datfile.size();

                m_datfile.Write((char*)&nnn, -1, SEEBLOCK);
            }
            block.eles[block.curNum].pos    = old;
            block.eles[block.curNum++].size = SEEBLOCK;
        }
    }
    if(block.checkSuitable(size, pos) == true)
    {
        PageEmptyBlock::PageEmptyEle ele = block.eles[pos];

        offset = ele.pos;
        ele.pos += size;
        ele.size -= size;

        for(int index = pos + 1; index < block.curNum; index++)
            block.eles[index - 1] = block.eles[index];

        if(ele.size == size) block.curNum--;
        else block.eles[block.curNum - 1] = ele;
    }
    else
    {
        if(block.curNum == PAGESIZE)
        {

            PageEmptyBlock nnn = block.split();
            nnn.nextBlock   = block.nextBlock;
            block.nextBlock = m_datfile.size();

            m_datfile.Append((char*)&nnn, SEEBLOCK);
        }

        char *str = new char[2*size];
        memset(str, 0, 2*size);
        m_datfile.Append(str, 2*size);
        delete str;
        offset  = m_datfile.size();
        offset -= size;

        block.eles[block.curNum].pos    = offset - size;
        block.eles[block.curNum++].size = size;
    }
    m_datfile.Write((char*)&block, fb, SEEBLOCK);
    return offset;
}

void     PageDB::printThisPage(PageTable * page)
{
    for(int i = 0; i < page -> curNum; i++)
    {
        BufferPacket packet(2*SINT);
        m_datfile.Read(packet.str(), page -> elements[i].m_datPos, packet.size());
        int a, b;
        packet >> a >> b;
        cout << a <<" " << b << " ";
    }
    cout << endl;
}

void     PageDB::fullAddLocalD(int cur, uint64_t num, uint64_t pos1, uint64_t pos2, uint64_t od)
{
    assert(entries.size() % num == 0);
    uint64_t each = ((uint64_t)entries.size()) / num;

    int ocur = cur;
    bool flag, oflag;

    if(((cur >> (od)) & 1) == 1) flag = true;
    else flag = false;
    oflag = flag;

    while(cur >= 0)
    {
        if(flag == true) entries[cur] = pos2;
        else entries[cur] = pos1;

        cur -= each;
        flag = !flag;
    }

    cur = ocur + each;
    flag = !oflag;

    while(cur < entries.size())
    {
        if(flag == true) entries[cur] = pos2;
        else entries[cur] = pos1;

        cur += each;
        flag = !flag;
    }
}

struct PageDB::Writer
{
    WriteBatch* batch;
    bool sync;
    bool done;
    CondVar cv;
    explicit Writer(Mutex* mu) : cv(mu) { }
};

WriteBatch * PageDB::BuildBatchGroup(Writer ** last_writer)
{
    Writer* first = m_writers.front();
    WriteBatch* result = first->batch;

    size_t size = result->getTotalsize();

    size_t max_size = 1 << 20;

    if (size <= (128<<10))
    {
        max_size = size + (128<<10);
    }

    *last_writer = first;

    deque<PageDB::Writer*>::iterator iter = m_writers.begin();

    for (++iter; iter != m_writers.end(); ++iter)
    {
        Writer* w = *iter;
        if (w->sync && !first->sync)
            break;

        if (w->batch != NULL)
        {
            size += WriteBatchInternal::Count(w->batch);
            if (size > max_size)
                break;

            if (result == first->batch)
            {
                result = m_tmpBatch;
                assert(result->getCount() == 0);
                WriteBatchInternal::Append(result, first->batch);
            }
            WriteBatchInternal::Append(result, w->batch);
        }
        *last_writer = w;
    }

    return result;
}

};
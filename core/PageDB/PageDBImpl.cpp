// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "PageDBImpl.h"

namespace pagedb
{

PageDB::PageDB(HashFunc hashFunc) : m_HashFunc(hashFunc),  m_gd(0), m_pn(1), m_fb(-1)
{
    m_cache = new PageCache(this);
    m_tmpBatch = new WriteBatch;
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

        m_gd = 0;
        m_pn = 1;
        m_fb = -1;
        m_entries.push_back(0);
        writeToIdxFile();

        PageTable * page = new PageTable(this);
        BufferPacket packet = page -> getPacket();

        m_datfile.Append(packet.c_str(), SPAGETABLE);

        m_cache -> put(page, 0);
    }
    else
    {
        /**
        ** Exist old DB, read index information from file
        **/
        readFromIdxFile();
    }

    /**
    ** If no empty block, allocate one from the end of file
    **/
    if(m_fb == -1)
    {
        m_fb = m_datfile.Size();

        PageEmptyBlock block;

        block.m_eles[0].m_pos  = m_fb + SEEBLOCK;
        block.m_eles[0].m_size = SEEBLOCK;
        block.m_curNum       = 1;

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

    if(m_tmpBatch)
        delete m_tmpBatch;

    return true;
}

bool     PageDB::put(const Slice & key,const Slice & value)
{
    WriteBatch batch(1);
    batch.put(key, value);

    return write(&batch);
}

Slice    PageDB::get(const Slice & key)
{
    uint32_t hashVal = m_HashFunc(key);
    uint32_t cur     = hashVal & ((1 << m_gd) -1);
    uint32_t index   = 0;
    uint64_t addr    = m_entries.at(cur) & MOD;

    PageTable * page = m_cache -> find(addr, index);

    if(page == NULL)
    {
        page = new PageTable(this);
        m_cache -> put(page, addr);

        readAndSetPage(page, addr);
    }

    Slice rs = page -> get(key, hashVal);

    return rs;
}

bool     PageDB::remove(const Slice & key)
{
    uint32_t hashVal = m_HashFunc(key);
    uint32_t cur     = hashVal & ((1 << m_gd) -1);
    uint32_t index   = 0;
    uint64_t addr    = m_entries.at(cur) & MOD;

    PageTable * page = m_cache -> find(addr, index);

    if(page == NULL)
    {
        page = new PageTable(this);
        index = m_cache -> put(page, addr);

        readAndSetPage(page, addr);
    }

    bool rb = page -> remove(key, hashVal);

    if(rb == true)
    {
        BufferPacket npacket = page -> getPacket();
        m_datfile.Read(npacket.str(), addr, npacket.size());

        m_cache -> updated(index);
    }
    else
        m_log -> _Warn("ExtendibleHash :: remove failed(maybe not exist?)");

    return rb;
}

/**
** Layer 2
**/
bool    PageDB::put(WriteBatch * pbatch)
{
    return write(pbatch);
}

/**
** Layer 3
**/

/**
** Force write to file?
**/
void    PageDB::sync()
{
    m_cache -> free();
    writeToIdxFile();
    m_datfile.Sync();
    m_idxfile.Sync();
}

/**
** Should dump to stream, Updated it
**/
void    PageDB::dump(ostream&os)
{
    sync();

    PageTable * page = new PageTable(this);

    os << "--------------------Basic Info--------------------" << endl;
    os << "global D" << m_gd << ", number of pages:" << m_pn << ", number of entries" << m_entries.size() << endl;
    os << "--------------------Page  Info--------------------" << endl;
    for(int cur = 0, index = 0; cur < m_entries.size(); cur++)
    {
        int j;
        for(j = 0; j < cur; j++)
        {
            if(m_entries.at(j) ==  m_entries.at(cur)) break;
        }

        if(j != cur) continue;

        uint64_t addr    = m_entries.at(cur) & MOD;

        readAndSetPage(page, addr);

        os << "Page(size:" << page -> m_curNum <<") " << index++ <<" "<< cur << " " << page -> m_d << ":";

        for(j = 0; j < page -> m_curNum; j++)
        {
            PageElement element = page -> m_elements[j];

            BufferPacket packet(element.m_keySize + element.m_datSize);
            m_datfile.Read(packet.str(), element.m_datPos, packet.size());
            Slice key(element.m_keySize), value(element.m_datSize);
            packet >> key >> value;
            uint32_t hashVal = page -> m_elements[j].m_hashVal;
            //    os << "[" << key << " " << value << " " << hashVal << "] ";
        }

        os << endl;
    }
    os << "--------------------End  Page--------------------" << endl;

    delete page;
    page = NULL;
}

void    PageDB::compact()
{
    sync();

    PageTable * page = new PageTable(this);

    RandomFile tmpfile1, tmpfile2;

    tmpfile1.Open("tmppage.bak");
    tmpfile2.Open("tmpcon.bak");
    tmpfile1.Truncate(0);
    tmpfile2.Truncate(0);

    WriteBatch * pbatch =  new WriteBatch(PAGESIZE*2);

    for(int cur = 0; cur < m_entries.size(); cur++)
    {
        int j;
        for(j = 0; j < cur; j++)
        {
            if(m_entries.at(j) ==  m_entries.at(cur)) break;
        }

        if(j != cur) continue;

        uint64_t addr    = m_entries.at(cur) & MOD;

        BufferPacket packet(SPAGETABLE);

        m_datfile.Read(packet.str(), addr, SPAGETABLE);
        page -> setByBucket(packet);

        for(j = 0; j < page -> m_curNum; j++)
        {
            PageElement element = page -> m_elements[j];

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

    vector<char> used(m_entries.size(), 0);

    uint32_t uds = 0;
    int pos1 = 0, pos2 = 0;
    for(int cur = 0; cur < m_entries.size(); cur++)
    {
        if(used.at(cur) == 0)
        {
            vector<int> ids;
            ids.push_back(cur);

            for(int j = cur + 1; j < m_entries.size(); j++)
            {
                if(m_entries.at(cur) == m_entries.at(j))
                {
                    used[j] = 1;
                    ids.push_back(j);
                }
            }
            // if(ids.size() == 3)
            // {
            //     printf("cur: %d\n", cur);
            //     for(int j = cur + 1; j < m_entries.size(); j++)
            //     {
            //         if(m_entries.at(cur) == m_entries.at(j))
            //         {
            //             printf("j: %d\n", j);
            //         }
            //     }
            // }

            // printf("ids Size, %d, %lld\n", ids.size(), (long long)m_entries.at(cur));
            if(ids.size() != 1) assert(is2Exp(ids.size()) == true);

            for(int j=0; j<ids.size(); j++)
            {
                uint64_t pageNum = ids.size();
                uint64_t k = 0;
                while(pageNum > 1)
                {
                    k++;
                    pageNum >>= 1;
                }
                m_entries[ids.at(j)] = uds | (k << 56);
            }

            BufferPacket packet(SPAGETABLE);
            tmpfile1.Read(packet.str(), pos1, SPAGETABLE);
            pos1 += SPAGETABLE;

            page -> setByBucket(packet);

            uint32_t size;
            tmpfile2.Read((char*)&size, pos2, sizeof(uint32_t));
            pos2 += sizeof(uint32_t);

            BufferPacket packet1(size);
            tmpfile2.Read(packet1.str(), pos2, packet1.size());
            pos2 += packet1.size();

            int fpos = uds + packet.size();
            for(int j=0; j<page->m_curNum; j++)
            {
                page->m_elements[j].m_datPos = fpos;
                fpos += page->m_elements[j].m_keySize + page->m_elements[j].m_datSize;
            }

            packet = page->getPacket();
            m_datfile.Append(packet.c_str(), packet.size());
            m_datfile.Append(packet1.c_str(), packet1.size());

            uds += packet.size() + packet1.size();
        }
    }

    m_fb = -1;
    writeToIdxFile();

    delete page;
    page = NULL;

    delete pbatch;
    pbatch = NULL;

    tmpfile1.Truncate(0);
    tmpfile2.Truncate(0);
    tmpfile1.Close();
    tmpfile2.Close();
    FileModule::Remove("tmppage.bak");
    FileModule::Remove("tmpcon.bak");
}

/**
** Internal functions
**/
void    PageDB::recycle(int offset, int size)
{
    ScopeMutex scope(&(m_datLock));

    PageEmptyBlock block;

    if(m_fb == -1)
    {
        m_log -> _Fatal("There must be some fatal error\n");
        return;
    }

    m_datfile.Read((char*)&block, m_fb, SEEBLOCK);
    if(block.m_curNum == PAGESIZE)
    {
        int nn = block.m_nextBlock;

        PageEmptyBlock nnBlock = block.split();
        nnBlock.m_nextBlock   = nn;
        block.m_nextBlock     = m_datfile.Size();

        m_datfile.Append((char*)&nnBlock, SEEBLOCK);
    }

    block.m_eles[block.m_curNum].m_pos    = offset;
    block.m_eles[block.m_curNum++].m_size = size;

    m_datfile.Write((char*)&block, m_fb, SEEBLOCK);
}

void     PageDB::writeToIdxFile()
{
    BufferPacket packet(SINT * 3 + m_entries.size()*SINT64);

    packet << m_gd << m_pn << m_fb;
    packet.write((char*)&m_entries[0], m_entries.size()*SINT64);

    m_idxfile.Write(packet.c_str(), 0, packet.size());
}

void     PageDB::readFromIdxFile()
{
    BufferPacket packet(SINT * 3);
    m_idxfile.Read(packet.str(), 0, packet.size());

    int gd1, pn1, fb1;
    packet >> gd1 >> pn1 >> fb1;

    m_gd = gd1;
    m_pn = pn1;
    m_fb = fb1;

    m_entries = vector<uint64_t> (1 << m_gd, 0);

    m_idxfile.Read((char*)&m_entries[0], SINT * 3, m_entries.size()*SINT64);
}

int      PageDB::findSuitableOffset(int size)
{
    assert(m_fb != -1);

    int offset, pos;

    PageEmptyBlock block;
    m_datfile.Read((char*)&block, m_fb, SEEBLOCK);

    if(block.m_nextBlock != -1)
    {
        if(block.m_curNum < PAGESIZE/2)
        {
            PageEmptyBlock nnBlock;

            int old = block.m_nextBlock;

            m_datfile.Write((char*)&nnBlock, block.m_nextBlock, SEEBLOCK);

            int index = 0;
            int bnum = block.m_curNum;

            block.m_nextBlock = nnBlock.m_nextBlock;

            for(; index < nnBlock.m_curNum; index++)
            {
                block.m_eles[block.m_curNum++] = nnBlock.m_eles[index];
                if(block.m_curNum == PAGESIZE)
                {
                    PageEmptyBlock nnn = block.split();
                    nnn.m_nextBlock   = block.m_nextBlock;
                    block.m_nextBlock = m_datfile.Size();

                    m_datfile.Write((char*)&nnn,-1, SEEBLOCK);
                }
            }

            if(block.m_curNum == PAGESIZE)
            {
                PageEmptyBlock nnn = block.split();
                nnn.m_nextBlock   = block.m_nextBlock;
                block.m_nextBlock = m_datfile.Size();

                m_datfile.Write((char*)&nnn, -1, SEEBLOCK);
            }
            block.m_eles[block.m_curNum].m_pos    = old;
            block.m_eles[block.m_curNum++].m_size = SEEBLOCK;
        }
    }
    if(block.find(size, pos) == true)
    {
        PageEmptyBlock::PageEmptyEle ele = block.m_eles[pos];

        offset = ele.m_pos;
        ele.m_pos += size;
        ele.m_size -= size;

        for(int index = pos + 1; index < block.m_curNum; index++)
            block.m_eles[index - 1] = block.m_eles[index];

        if(ele.m_size == size) block.m_curNum--;
        else block.m_eles[block.m_curNum - 1] = ele;
    }
    else
    {
        if(block.m_curNum == PAGESIZE)
        {

            PageEmptyBlock nnn = block.split();
            nnn.m_nextBlock   = block.m_nextBlock;
            block.m_nextBlock = m_datfile.Size();

            m_datfile.Append((char*)&nnn, SEEBLOCK);
        }

        char *str = new char[2*size];
        memset(str, 0, 2*size);
        m_datfile.Append(str, 2*size);
        delete str;
        offset  = m_datfile.Size();
        offset -= size;

        block.m_eles[block.m_curNum].m_pos    = offset - size;
        block.m_eles[block.m_curNum++].m_size = size;
    }
    m_datfile.Write((char*)&block, m_fb, SEEBLOCK);
    return offset;
}

void     PageDB::fullAddLocalD(int cur, uint64_t num, uint64_t pos1, uint64_t pos2, uint64_t od)
{
    assert(m_entries.size() % num == 0);

    uint64_t each = ((uint64_t)m_entries.size()) / num;

    int ocur = cur;
    bool flag, oflag;

    if(((cur >> (od)) & 1) == 1) flag = true;
    else flag = false;
    oflag = flag;

    while(cur >= 0)
    {
        if(flag == true) m_entries[cur] = pos2;
        else m_entries[cur] = pos1;

        cur -= each;
        flag = !flag;
    }

    cur = ocur + each;
    flag = !oflag;

    while(cur < m_entries.size())
    {
        if(flag == true) m_entries[cur] = pos2;
        else m_entries[cur] = pos1;

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

WriteBatch * PageDB::buildBatchGroup(Writer ** last_writer)
{
    Writer* first      = m_writers.front();
    WriteBatch* result = first->batch;

    size_t size = WriteBatchInternal::ByteSize(result);

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

bool     PageDB::write(WriteBatch* pbatch)
{
    if(pbatch == NULL) return false;

    Writer w(&m_writerlock);

    w.batch = pbatch;
    w.sync  = false;
    w.done  = false;

    ScopeMutex l(&m_writerlock);

    m_writers.push_back(&w);

    while (!w.done && &w != m_writers.front())
    {
        w.cv.wait();
    }

    if (w.done)
    {
        return true;
    }

    Writer* last_writer = &w;
    WriteBatch* updates = buildBatchGroup(&last_writer);

    {
        m_writerlock.unlock();
        /**
        ** only one thread can access it at the same time.
        **/
        writeBatch(updates);
        m_writerlock.lock();
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

    return true;
}

bool     PageDB::writeBatch(WriteBatch * pbatch)
{
    uint32_t curpos = m_datfile.Size();

    BufferPacket phyPacket(WriteBatchInternal::ByteSize(pbatch));
    m_datfile.Append(phyPacket.c_str(), phyPacket.size());

    uint32_t totalSize = 0;
    WriteBatch::Iterator iterator(pbatch);

    for(const Node * node = iterator.first(); node != iterator.end(); node = iterator.next())
    {
        Slice key   = node -> first;
        Slice value = node -> second;

        uint32_t hashVal = m_HashFunc(key);
        uint32_t cur     = hashVal & ((1 << m_gd) -1);
        uint32_t index   = 0;

        uint64_t addr    =  m_entries.at(cur) & MOD;
        uint64_t digNum  = (m_entries.at(cur) & (~MOD));
        uint64_t pageNum = (digNum >> 56);

        PageTable * page = m_cache -> find(addr, index);

        if(page == NULL)
        {
            page  = new PageTable(this);
            index = m_cache -> put(page, addr);

            BufferPacket packet(SPAGETABLE);

            m_datfile.Read(packet.str(), addr, SPAGETABLE);

            page -> setByBucket(packet);
        }

        if(page -> full() && page -> m_d == m_gd)
        {
            m_gd++;

            int oldSize = m_entries.size();
            for(int i = 0; i < oldSize; i++)
            {
                uint64_t addr    = m_entries.at(i) & MOD;
                uint64_t pageNum = m_entries.at(i) & (~MOD);
                pageNum >>= 56;
                pageNum++;
                pageNum <<= 56;
                m_entries[i] = addr | pageNum;
            }

            for(int i = 0; i < oldSize; i++)
                m_entries.push_back(m_entries.at(i));

            pageNum++;
        }

        if(page -> full() && page -> m_d < m_gd)
        {
            page -> put(key, value, hashVal, totalSize + curpos);
            phyPacket << key << value;
            totalSize += key.size() + value.size();

            m_cache -> updated(index);

            PageTable * p2 = new PageTable(this);

            int index = 0, curNum2 = 0, curNum3 = 0;

            for(index = 0; index < page -> m_curNum; index++)
            {
                PageElement element = page -> m_elements[index];

                int id = element.m_hashVal;

                int flag = (id & ((1 << m_gd) - 1));

                if(((flag >> (page -> m_d)) & 1) == 1)
                    p2 -> m_elements[curNum3++] = page -> m_elements[index];
                else
                    page -> m_elements[curNum2++] = page -> m_elements[index];
            }

            page -> m_curNum = curNum2;
            p2   -> m_curNum = curNum3;

            uint64_t oldpos = addr;
            uint64_t oldpos2 = m_datfile.Size();

            assert(pageNum - 1 >= 0);
            pageNum--;
            digNum = pageNum << 56;

            oldpos2 = oldpos2 | digNum;
            oldpos  = oldpos  | digNum;

            uint64_t pageNum2 = 1ull << (pageNum + 1);
            fullAddLocalD(cur, pageNum2, oldpos, oldpos2, page -> m_d);

            page -> m_d = p2 -> m_d = (page -> m_d) + 1;

            BufferPacket packe2 = p2 -> getPacket();
            m_datfile.Append(packe2.c_str(), SPAGETABLE);

            m_pn += 1;

            delete p2;
            p2 = NULL;
        }
        else
        {
            page -> put(key, value, hashVal, totalSize + curpos);
            phyPacket << key << value;
            totalSize = totalSize + key.size() + value.size();

            m_cache -> updated(index);
        }
    }

    m_datfile.Write(phyPacket.c_str(), curpos, phyPacket.size());

    writeToIdxFile();
    m_cache -> free();

    return true;
}

void PageDB::readAndSetPage(PageTable *page, uint64_t addr)
{
    BufferPacket packet(SPAGETABLE);
    m_datfile.Read(packet.str(), addr, SPAGETABLE);
    page -> setByBucket(packet);
}

bool PageDB::is2Exp(uint64_t val)
{
    return ((val != 0) && (val & (val - 1)) == 0);
}

};
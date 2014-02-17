#include "FactoryImpl.h"
#include "Utils.h"

#define FILEMODE1 (fstream::in | fstream::out)

#define FILEMODE2 (fstream::out | fstream::app)

static uint32_t datfileLen = 0;
typedef pair<Slice, Slice> Node;

bool EEmptyBlock::checkSuitable(int size, int & pos)
{
    for(pos = curNum - 1; pos >= 0; pos--)
    {
        if(eles[pos].size > size)
            return true;
    }
    return false;
}

EEmptyBlock EEmptyBlock::split()
{
    EEmptyBlock newblock;

    int cn1 = 0, cn2 = 0;

    for(int index = 0; index < curNum; index++)
    {
        if(index & 1) newblock.eles[cn1++] = eles[index];
        else eles[cn2++] = eles[index];
    }

    newblock.curNum = cn1;
    this -> curNum = cn2;

    return newblock;
}

BufferPacket Page::getPacket()
{
    BufferPacket packet(SINT * 2 + sizeof(elements));

    Slice slice((char*)&elements[0], sizeof(elements));

    packet << d << curNum << slice;

    return packet;
}

void  Page::setByBucket(BufferPacket & packet)
{
    packet.setBeg();

    int d1, curNum1;
    packet >> d1 >> curNum1;
    if(curNum1 > 100) cout<<d1<<" "<<curNum1<<endl;
    d = d1;
    curNum = curNum1;
    packet.read((char*)&elements[0],sizeof(elements));
}

bool Page::put(const Slice & key, const Slice & value, uint32_t hashVal)
{
    for(int index = 0; index < curNum; index++)
    {
        PageElement element = elements[index];
        if(element.m_hashVal == hashVal && element.m_keySize == \
                key.size() && element.m_datSize == value.size())
        {
            BufferPacket packet(element.m_keySize);
            Slice        slice(element.m_keySize);

            eHash -> datfs.seekg(element.m_datPos, ios_base::beg);
            eHash -> datfs.read(packet.getData(), packet.getSize());

            packet.setBeg();
            packet >> slice;

            if(slice == key)
                return 0;
        }
    }
    /**
      Find an suitable empty block, if not allocated it at the end of the file
    **/
    BufferPacket packet(key.size() + value.size());
    packet << key << value;

    int offset = eHash -> findSuitableOffset(packet.getSize());

    eHash -> datfs.seekg(offset, ios_base::beg);
    eHash -> datfs.write(packet.getData(), packet.getSize());

    /**
        Modify the page index
    **/

    elements[curNum].m_hashVal   = hashVal;
    elements[curNum].m_datPos    = offset;
    elements[curNum].m_keySize   = key.size();
    elements[curNum++].m_datSize = value.size();

    return 1;
}

void Page::replaceQ(const Slice & key, const Slice & value, uint32_t hashVal, int offset)
{
    for(int index = 0; index < curNum; index++)
    {
        PageElement element = elements[index];
        if(element.m_hashVal == hashVal && element.m_keySize == \
                key.size() && element.m_datSize == value.size())
        {

            BufferPacket packet(element.m_keySize);
            Slice        slice(element.m_keySize);

            {
                ScopeMutex scope(&(eHash -> datLock));
                eHash -> datfs.seekg(element.m_datPos, ios_base::beg);
                eHash -> datfs.read(packet.getData(), packet.getSize());
            }

            packet.setBeg();
            packet >> slice;
            if(slice != key)
            {
                printf("notice3\n");
                continue;
            }

            eHash -> recycle(elements[index].m_datPos, key.size() + value.size());

            elements[index].m_datPos  = offset;
            elements[index].m_datSize = value.size();
            return;
        }
    }

    elements[curNum].m_hashVal   = hashVal;
    elements[curNum].m_datPos    = offset;
    elements[curNum].m_keySize   = key.size();
    elements[curNum++].m_datSize = value.size();
}

Slice Page::get(const Slice & key, uint32_t hashVal)
{
    for(int index = 0; index < curNum; index++)
    {
        if(elements[index].m_hashVal == hashVal && elements[index].m_keySize == key.size())
        {
            BufferPacket packet(elements[index].m_datSize + elements[index].m_keySize);

            Slice slice1(elements[index].m_datSize);
            Slice slice2(elements[index].m_keySize);

            eHash -> datfs.seekg(elements[index].m_datPos, ios_base::beg);

            eHash -> datfs.read(packet.getData(), packet.getSize());

            packet >> slice2 >> slice1;

            if(slice2 == key)
                return slice1;
        }
    }
    return "";
}

bool Page::remove(const Slice & key, uint32_t hashVal)
{
    int index;
    for(index = 0; index < curNum; index++)
    {
        if(elements[index].m_hashVal == hashVal && elements[index].m_keySize == key.size())
        {
            BufferPacket packet(elements[index].m_keySize);
            Slice slice(elements[index].m_keySize);

            eHash -> datfs.seekg(elements[index].m_datPos, ios_base::beg);
            eHash -> datfs.read(packet.getData(), packet.getSize());

            packet >> slice;

            if(slice == key) break;
        }
    }

    if(index == curNum) return false;

    /**
      Attach the space to the emptryBlock
    **/
    eHash -> recycle(elements[index].m_datPos, elements[index].m_keySize + elements[index].m_datSize);

    for(; index < curNum - 1; index++)
        elements[index] = elements[index + 1];

    curNum --;

    return true;
}

ExtendibleHash::ExtendibleHash(HASH hashFunc) :\
    hashFunc(hashFunc), gd(0), pn(1), fb(-1), globalLock(&tmplock), notnotnot(0), MOD((1ull << 56) - 1)
{
    pcache = new PageCache(this);
    m_tmpBatch = new WriteBatch;
    m_totalTime.tv_sec = 0;
    m_totalTime.tv_usec = 0;
}

ExtendibleHash::~ExtendibleHash()
{
    fflush();

    if(idxfs) writeToIdxFile();
    if(idxfs) idxfs.close();
    if(datfs) datfs.close();
    if(pcache) delete pcache;
    if(m_tmpBatch) delete m_tmpBatch;
}

void ExtendibleHash::fflush()
{
    printf("globalTimes %d\n",globalTimes);
    printf("Disk time %d %d\n",m_totalTime.tv_sec, m_totalTime.tv_usec);

    pcache -> free();

    {
        ScopeMutex scope(&datLock);
        datfs.flush();
    }

    {
        globalLock.writeLock();
        idxfs.flush();
        globalLock.writeUnlock();
    }
}

bool ExtendibleHash::init(const char * filename)
{
    struct stat buf;

    string sfilename(filename, filename + strlen(filename));
    idxName = sfilename + ".idx";
    datName = sfilename +  ".dat";

    /**Sorry for that, I don't find better solution.**/
    idxfs.open (idxName.c_str(), FILEMODE2);
    datfs.open (datName.c_str(), FILEMODE2);

    idxfs.close();
    datfs.close();

    idxfs.open (idxName.c_str(), FILEMODE1);
    datfs.open (datName.c_str(), FILEMODE1);

    if(!idxfs || !datfs)
    {
        log -> _Fatal("ExtendibleHash::init::open error\n");

        return false;
    }

    if((stat(idxName.c_str(), &buf) == -1) || buf.st_size == 0)
    {
        gd = 0;
        pn = 1;

        entries.push_back(0);
        /**Can use MemoryPool here**/

        Page * page = new Page(this);

        idxfs.seekg(0,ios_base::beg);
        writeToIdxFile();

        datfs.seekg(0,ios_base::beg);
        uint32_t pos = datfs.tellg();

        BufferPacket packet = page -> getPacket();
        datfs.write(packet.getData(),packet.getSize());

        pcache -> putInto(page, pos);
    }
    else
    {
        readFromFile();
        datfs.seekg(0, ios_base::end);
    }

    if(fb == -1)
    {
        datfs.seekg(0, ios_base::end);
        fb = datfs.tellg();

        EEmptyBlock block;

        block.eles[0].pos  = fb + SEEBLOCK;
        block.eles[0].size = SEEBLOCK;
        block.curNum       = 1;

        datfs.write((char*)&block, SEEBLOCK);
        datfs.write((char*)&block, SEEBLOCK);
    }

    return true;
}

void ExtendibleHash::writeToIdxFile()
{
    idxfs.seekg(0,ios_base::beg);

    BufferPacket packet(SINT * 3 + entries.size()*SINT64);
    packet << gd << pn << fb;
    packet.write((char*)&entries[0], entries.size()*SINT64);

    idxfs.write(packet.getData(),packet.getSize());
}

void ExtendibleHash::readFromFile()
{
    idxfs.seekg(0,ios_base::beg);

    BufferPacket packet(SINT * 3);
    idxfs.read(packet.getData(), packet.getSize());

    int gd1, pn1, fb1;
    packet >> gd1 >> pn1 >> fb1;
    gd = gd1;
    pn = pn1;
    fb = fb1;
    entries = vector<uint64_t> (1 << gd, 0);

    idxfs.read((char*)&entries[0], entries.size()*SINT64);
}

bool ExtendibleHash::put(const Slice & key,const Slice & value)
{
    uint32_t hashVal = hashFunc(key);

    int cur     = hashVal & ((1 << gd) -1);

    int index = 0;

    uint64_t addr    = entries.at(cur) & MOD;
    uint64_t digNum  = (entries.at(cur) & (~MOD));
    uint64_t pageNum = (digNum >> 56);
    Page * page = pcache -> find(addr, index);
    bool entriesUpdate = false;

    if(page == NULL)
    {
        page = new Page(this);
        index = pcache -> putInto(page, addr);

        BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));

        datfs.seekg(addr, ios_base::beg);
        datfs.read(packet.getData(), packet.getSize());

        page -> setByBucket(packet);
        assert(page -> curNum < PAGESIZE + 5);
    }

    if(page -> full() && page -> d == gd)
    {
        log -> _Trace("ExtendibleHash :: put :: full, so double-entries\n");
        printf("double\n");
        gd += 1;
        pn = 2*pn;

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
        globalTimes++;
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

        Page * p2 = new Page(this);

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

        datfs.seekg(0, ios_base::end);

        uint64_t oldpos  = addr;
        uint64_t oldpos2 = datfs.tellg();

        assert(pageNum - 1 >= 0);
        pageNum--;
        digNum = pageNum << 56;

        oldpos2 = oldpos2 | digNum;
        oldpos  = oldpos  | digNum;

        // for(index = 0; index < entries.size(); index++)
        // {
        //     uint64_t addr1 = entries.at(index) & MOD;
        //     if(addr1 == oldpos)
        //     {
        //         if(((index >> (page -> d)) & 1) == 1)
        //             entries[index] = oldpos2 | digNum;
        //         else
        //             entries[index] = oldpos | digNum;
        //     }
        // }
        uint64_t pageNum2 = 1ull << (pageNum + 1);
        fullAddLocalD(cur, pageNum2, oldpos, oldpos2, page -> d);

        page -> d = p2 -> d = (page -> d) + 1;

        BufferPacket packe2 = p2 -> getPacket();
        datfs.write(packe2.getData(), packe2.getSize());

        delete p2;
        p2 = NULL;
    }
    else
    {
        if((page -> put(key, value, hashVal)) == 1)
        {
            pcache -> setUpdated(index);
        }
        else
        {
            log -> _Warn("ExtendibleHash :: put :: HAS been in the file\n");
            return false;
        }
    }
    datfs.flush();
    writeToIdxFile();

    return true;
}

Slice ExtendibleHash::get(const Slice & key)
{
    uint32_t hashVal = hashFunc(key);
    int cur = hashVal & ((1 << gd) -1);

    int index = 0;

    uint64_t addr    =  entries.at(cur) & MOD;

    Page * page = pcache -> find(addr, index);

    if(page == NULL)
    {
        page = new Page(this);
        pcache -> putInto(page, addr);

        BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));

        datfs.seekg(addr, ios_base::beg);
        datfs.read(packet.getData(), packet.getSize());

        page -> setByBucket(packet);
    }

    /* printThisPage(page);*/

    Slice rs = page -> get(key, hashVal);

    return rs;
}

bool ExtendibleHash::remove(const Slice & key)
{
    uint32_t hashVal = hashFunc(key);
    int cur   = hashVal & ((1 << gd) -1);

    int index = 0;

    uint64_t addr    =  entries.at(cur) & MOD;

    Page * page = pcache -> find(addr, index);

    if(page == NULL)
    {
        page = new Page(this);
        index = pcache -> putInto(page, addr);

        BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));

        datfs.seekg(addr, ios_base::beg);
        datfs.read(packet.getData(), packet.getSize());

        page -> setByBucket(packet);
    }

    int rb = page -> remove(key, hashVal);

    if(rb == 1)
    {
        BufferPacket npacket = page -> getPacket();

        datfs.seekg(addr, ios_base::beg);
        datfs.write(npacket.getData(), npacket.getSize());

        pcache -> setUpdated(index);
    }
    else
        log -> _Trace("ExtendibleHash :: remove failed(maybe not exist?)");

    return rb;
}

int ExtendibleHash::findSuitableOffset(int size)
{
    EEmptyBlock block;
    int offset, pos;
    assert(fb != -1);

    datfs.seekg(fb, ios_base::beg);
    datfs.read((char*)&block, SEEBLOCK);

    if(block.nextBlock != -1)
    {
        if(block.curNum < PAGESIZE/2)
        {
            EEmptyBlock nnBlock;

            int old = block.nextBlock;

            datfs.seekg(block.nextBlock, ios_base::beg);
            datfs.read((char*)&nnBlock, SEEBLOCK);

            int index = 0;
            int bnum = block.curNum;

            block.nextBlock = nnBlock.nextBlock;

            datfs.seekg(0, ios_base::end);

            for(; index < nnBlock.curNum; index++)
            {
                block.eles[block.curNum++] = nnBlock.eles[index];
                if(block.curNum == PAGESIZE)
                {
                    EEmptyBlock nnn = block.split();
                    nnn.nextBlock   = block.nextBlock;
                    block.nextBlock = datfs.tellg();

                    datfs.write((char*)&nnn, SEEBLOCK);
                }
            }

            if(block.curNum == PAGESIZE)
            {
                EEmptyBlock nnn = block.split();
                nnn.nextBlock   = block.nextBlock;
                block.nextBlock = datfs.tellg();

                datfs.write((char*)&nnn, SEEBLOCK);
            }
            block.eles[block.curNum].pos    = old;
            block.eles[block.curNum++].size = SEEBLOCK;
        }
    }

    if(block.checkSuitable(size, pos) == true)
    {
        EEmptyBlock::EEmptyEle ele = block.eles[pos];

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
            datfs.seekg(0, ios_base::end);

            EEmptyBlock nnn = block.split();
            nnn.nextBlock   = block.nextBlock;
            block.nextBlock = datfs.tellg();

            datfs.write((char*)&nnn,SEEBLOCK);
        }

        datfs.seekg(2 * size,ios_base::end);

        offset = datfs.tellg();
        offset -= size;

        block.eles[block.curNum].pos    = offset - size;
        block.eles[block.curNum++].size = size;
    }

    datfs.seekg(fb, ios_base::beg);
    datfs.write((char*)&block, SEEBLOCK);

    return offset;
}

void ExtendibleHash::recycle(int offset, int size)
{
    ScopeMutex scope(&(datLock));

    EEmptyBlock block;

    if(fb == -1)
    {
        log -> _Fatal("There must be some fatal error\n");
        return;
    }

    datfs.seekg(fb,      ios_base::beg);
    datfs.read((char*)&block, SEEBLOCK);

    if(block.curNum == PAGESIZE)
    {
        datfs.seekg(0, ios_base::end);

        int nn = block.nextBlock;

        EEmptyBlock nnBlock = block.split();
        nnBlock.nextBlock   = nn;
        block.nextBlock     = datfs.tellg();

        datfs.write((char*)&nnBlock, SEEBLOCK);
    }

    block.eles[block.curNum].pos    = offset;
    block.eles[block.curNum++].size = size;

    datfs.seekg(fb, ios_base::beg);
    datfs.write((char*)&block,SEEBLOCK);

    datfs.flush();
}

void ExtendibleHash::dump()
{
    pcache -> fflush();

    Page * page = new Page(this);
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

        BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));
        datfs.seekg(addr, ios_base::beg);
        datfs.read(packet.getData(), packet.getSize());

        page -> setByBucket(packet);

        cout << "Page(size:"<<page -> curNum <<") " << index++ <<" "<<cur <<" "<<page -> d<<":";

        for(j = 0; j < page -> curNum; j++)
        {
            datfs.seekg(page -> elements[j].m_datPos, ios_base::beg);
            BufferPacket packet(2*SINT);
            datfs.read(packet.getData(),packet.getSize());
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

void ExtendibleHash::printThisPage(Page * page)
{
    for(int j = 0; j < page -> curNum; j++)
    {
        datfs.seekg(page -> elements[j].m_datPos, ios_base::beg);
        BufferPacket packet(2*SINT);
        datfs.read(packet.getData(),packet.getSize());
        int a, b;
        packet >> a >> b;
        cout << a <<" " << b << " ";
    }
    cout << endl;
}

void ExtendibleHash::runBatch(const WriteBatch * pbatch)
{
    m_ts.StartTime();
    datfs.seekg(0, ios_base::end);
    uint32_t curpos = datfs.tellg();

    BufferPacket phyPacket(pbatch->getTotalSize());
    datfs.write(phyPacket.getData(), phyPacket.getSize());
    m_ts.StopTime();
    m_ts.AddTime(m_totalTime);

    uint32_t totalSize = 0;
    WriteBatch::Iterator iterator(pbatch);

    for(const Node * node = iterator.first(); node != iterator.end(); node = iterator.next())
    {
        Slice key   = node -> first;
        Slice value = node -> second;

        uint32_t hashVal = hashFunc(key);
        int cur   = hashVal & ((1 << gd) -1);
        int index = 0;

        uint64_t addr    =  entries.at(cur) & MOD;
        uint64_t digNum  = (entries.at(cur) & (~MOD));
        uint64_t pageNum = (digNum >> 56);

        Page * page = pcache -> find(addr, index);

        if(page == NULL)
        {
            page  = new Page(this);
            index = pcache -> putInto(page, addr);

            BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));
            m_ts.StartTime();
            datfs.seekg(addr, ios_base::beg);
            datfs.read(packet.getData(), packet.getSize());
            m_ts.StopTime();
            m_ts.AddTime(m_totalTime);
            page -> setByBucket(packet);
        }

        if(page -> full() && page -> d == gd)
        {
            gd++;
            pn = 2*pn;

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

            Page * p2 = new Page(this);

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
            m_ts.StartTime();
            datfs.seekg(0, ios_base::end);

            uint64_t oldpos = addr;
            uint64_t oldpos2 = datfs.tellg();

            assert(pageNum - 1 >= 0);
            pageNum--;
            digNum = pageNum << 56;

            oldpos2 = oldpos2 | digNum;
            oldpos  = oldpos  | digNum;

            uint64_t pageNum2 = 1ull << (pageNum + 1);
            fullAddLocalD(cur, pageNum2, oldpos, oldpos2, page -> d);

            page -> d = p2 -> d = (page -> d) + 1;

            BufferPacket packe2 = p2 -> getPacket();
            datfs.write(packe2.getData(), packe2.getSize());
            m_ts.StopTime();
            m_ts.AddTime(m_totalTime);
        }
        else
        {
            page -> replaceQ(key, value, hashVal, totalSize + curpos);
            phyPacket << key << value;
            totalSize = totalSize + key.size() + value.size();

            pcache -> setUpdated(index);
        }
    }
    m_ts.StartTime();
    datfs.seekg(curpos, ios_base::beg);
    datfs.write(phyPacket.getData(), phyPacket.getSize());
    m_ts.StopTime();
    m_ts.AddTime(m_totalTime);
    writeToIdxFile();
    pcache -> free();
}

struct ExtendibleHash::Writer
{
    WriteBatch* batch;
    bool sync;
    bool done;
    CondVar cv;
    explicit Writer(Mutex* mu) : cv(mu) { }
};

void ExtendibleHash::write(WriteBatch * pbatch)
{
    if(pbatch == NULL) return;

    Writer w(&m_mutex);
    w.batch = pbatch;
    w.sync = false;/*as I don't write sync option in Option.h, so just assume it is false*/
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

WriteBatch* ExtendibleHash::BuildBatchGroup(Writer ** last_writer)
{
    Writer* first = m_writers.front();
    WriteBatch* result = first->batch;

    size_t size = result->getTotalSize();

    size_t max_size = 1 << 20;

    if (size <= (128<<10))
    {
        max_size = size + (128<<10);
    }

    *last_writer = first;

    deque<ExtendibleHash::Writer*>::iterator iter = m_writers.begin();

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

void  ExtendibleHash::compact()
{
    pcache -> fflush();

    datfs.flush();
    idxfs.flush();

    Page * page = new Page(this);

    FILE * tmpfile1 = fopen("tmppage.bak","w+");
    FILE * tmpfile2 = fopen("tmpcon.bak","w+");
    assert(tmpfile1 && tmpfile2);

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

        BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));
        datfs.seekg(addr, ios_base::beg);
        datfs.read(packet.getData(), packet.getSize());

        page -> setByBucket(packet);

        for(j = 0; j < page -> curNum; j++)
        {
            PageElement element = page -> elements[j];

            datfs.seekg(element.m_datPos, ios_base::beg);
            BufferPacket packet1(element.m_keySize + element.m_datSize);
            datfs.read(packet1.getData(),packet1.getSize());
            Slice a(element.m_keySize), b(element.m_datSize);
            packet1 >> a >> b;
            pbatch -> put(a,b);
        }

        int nmeb = fwrite(packet.getData(), packet.getSize(), 1, tmpfile1);
        assert(nmeb == 1);
        BufferPacket packet1(WriteBatchInternal::ByteSize(pbatch));
        WriteBatch::Iterator iter(pbatch);
        for(const Node * node = iter.first(); node != iter.end(); node = iter.next())
        {
            packet1 << (node -> first) << (node -> second);
        }
        uint32_t size = packet1.getSize();

        fwrite((char*)&(size), sizeof(uint32_t), 1, tmpfile2);
        nmeb = fwrite(packet1.getData(), packet1.getSize(), 1, tmpfile2);
        assert(nmeb == 1);

        pbatch -> clear();
    }

    idxfs.close();
    datfs.close();

    string nidxName = "rm " + idxName;
    string ndatName = "rm " + datName;
    system(nidxName.c_str());
    system(ndatName.c_str());

    /**Sorry for that, I don't find better solution.**/
    idxfs.open (idxName.c_str(), FILEMODE2);
    datfs.open (datName.c_str(), FILEMODE2);

    idxfs.close();
    datfs.close();

    idxfs.open (idxName.c_str(), FILEMODE1);
    datfs.open (datName.c_str(), FILEMODE1);

    vector<char> used(entries.size(), 0);

    fseek(tmpfile1,0,SEEK_SET);
    fseek(tmpfile2,0,SEEK_SET);

    fclose(tmpfile1);
    fclose(tmpfile2);
    tmpfile1 = fopen("tmppage.bak","rb+");
    tmpfile2 = fopen("tmpcon.bak","rb+");
    assert(tmpfile1 && tmpfile2);

    fseek(tmpfile1, 0, SEEK_SET);
    fseek(tmpfile2, 0, SEEK_SET);

    assert(ftell(tmpfile1) == 0);
    assert(ftell(tmpfile2) == 0);

    uint32_t uds = 0;
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

            BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));
            int nmeb = fread(packet.getData(), packet.getSize(), 1, tmpfile1);

            assert(GetFileLen(tmpfile1) != 0);
            assert(nmeb == 1);

            page -> setByBucket(packet);

            uint32_t size;
            nmeb = fread((char*)&size, sizeof(uint32_t), 1, tmpfile2);
            assert(nmeb == 1);

            BufferPacket packet1(size);
            nmeb = fread(packet1.getData(), packet1.getSize(), 1, tmpfile2);
            assert(nmeb == 1);

            int fpos = uds + packet.getSize();
            for(int j=0; j<page->curNum; j++)
            {
                page->elements[j].m_datPos = fpos;
                fpos += page->elements[j].m_keySize + page->elements[j].m_datSize;
            }

            packet = page->getPacket();
            datfs.write(packet.getData(), packet.getSize());
            datfs.write(packet1.getData(), packet1.getSize());

            uds += packet.getSize() + packet1.getSize();
        }
    }

    this -> fb = -1;
    datfs.flush();
    writeToIdxFile();
    idxfs.flush();

    fclose(tmpfile1);
    fclose(tmpfile2);

    delete page;
    page = NULL;
    delete pbatch;
    pbatch = NULL;

    nidxName = "rm tmppage.bak";
    ndatName = "rm tmpcon.bak";
    system(nidxName.c_str());
    system(ndatName.c_str());
}

void ExtendibleHash::runBatch2(const WriteBatch * pbatch)
{
    BufferPacket phyPacket(pbatch->getTotalSize());

    uint32_t curpos = 0, totalSize = 0;
    Page * page;

    {
        ScopeMutex scope(&datLock);
        m_ts.StartTime();

        datfs.seekg(0, ios_base::end);
        curpos = datfs.tellg();
        datfs.write(phyPacket.getData(), phyPacket.getSize());

        m_ts.StopTime();
        m_ts.AddTime(m_totalTime);
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
            printf("oh my.god\n");
            globalLock.writeLock();
        }

        int cur   = hashVal & ((1 << gd) -1);
        assert((1<<gd) == entries.size());
        int index = 0;

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
            page  = new Page(this);

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

            BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));

            {
                ScopeMutex lock(&datLock);
                m_ts.StartTime();
                assert(cur < entries.size());
                datfs.seekg(addr, ios_base::beg);
                datfs.read(packet.getData(), packet.getSize());
                m_ts.StopTime();
                m_ts.AddTime(m_totalTime);
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
            pn = 2*pn;

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
            globalTimes++;

            page -> replaceQ(key, value, hashVal, totalSize + curpos);
            phyPacket << key << value;
            totalSize += key.size() + value.size();

            pcache -> setUpdated(index2);

            Page * p2 = new Page(this);

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
                m_ts.StartTime();
                datfs.seekg(0, ios_base::end);
                oldpos2 = datfs.tellg();
                datfs.write(packe2.getData(), packe2.getSize());
                m_ts.StopTime();
                m_ts.AddTime(m_totalTime);
            }

            uint64_t oldpos = addr;

            assert(pageNum - 1 >= 0);
            pageNum--;
            digNum = pageNum << 56;

            // for(index = 0; index < entries.size(); index++)
            // {
            //     uint64_t addr1 = entries.at(index) & MOD;
            //     if(addr1 == oldpos)
            //     {
            //         if(((index >> (od)) & 1) == 1)
            //             entries[index] = oldpos2 | digNum;
            //         else
            //             entries[index] = oldpos | digNum;
            //     }
            // }

            oldpos2 = oldpos2 | digNum;
            oldpos  = oldpos  | digNum;

            uint64_t pageNum2 = 1ull << (pageNum + 1);
            fullAddLocalD(cur, pageNum2, oldpos, oldpos2, od);

            delete p2;
            p2 = NULL;
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
        m_ts.StartTime();
        datfs.seekg(curpos, ios_base::beg);
        datfs.write(phyPacket.getData(), phyPacket.getSize());
        m_ts.StopTime();
        m_ts.AddTime(m_totalTime);
    }
}

void   ExtendibleHash::fullAddLocalD(int cur, uint64_t num, uint64_t pos1, uint64_t pos2, uint64_t od)
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
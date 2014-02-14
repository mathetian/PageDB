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

    for(int index = 0;index < curNum;index++)
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
    packet >> d >> curNum;
    
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
            /**
              Read From File, check it whether double
            **/
            
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

    // assert(eHash -> datfs.tellg() == datfileLen);

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
            
            Slice slice1(elements[index].m_datSize); Slice slice2(elements[index].m_keySize);

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

    for(;index < curNum - 1; index++)
        elements[index] = elements[index + 1];
    
    curNum --;

    return true;
}

ExtendibleHash::ExtendibleHash(HASH hashFunc) :\
        hashFunc(hashFunc), gd(0), pn(1), fb(-1), globalLock(&tmplock) { pcache = new PageCache(this); m_tmpBatch = new WriteBatch; }

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
    pcache -> free();
    
    {
        ScopeMutex scope(&datLock);
        datfs.flush();
    }

    {
       // ScopeMutex scope(&globalLock);
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
    
    idxfs.close(); datfs.close();

    idxfs.open (idxName.c_str(), FILEMODE1);
    datfs.open (datName.c_str(), FILEMODE1);
    
    if(!idxfs || !datfs)
    {
        log -> _Fatal("ExtendibleHash::init::open error\n");
       
        return false;
    }

    if((stat(idxName.c_str(), &buf) == -1) || buf.st_size == 0)
    {
        gd = 0; pn = 1;
        
        entries.push_back(0);
        /**Can use MemoryPool here**/

        Page * page = new Page(this);

        idxfs.seekg(0,ios_base::beg);
        writeToIdxFile();

        datfs.seekg(0,ios_base::beg);
        uint32_t pos = datfs.tellg();

        BufferPacket packet = page -> getPacket();
        datfs.write(packet.getData(),packet.getSize());

        datfileLen = datfs.tellg();
        pcache -> putInto(page, pos);
    }
    else
    {
        readFromFile();
        datfs.seekg(0, ios_base::end);
        datfileLen = datfs.tellg();
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
    
    BufferPacket packet(SINT * 3 + entries.size()*SINT);
    packet << gd << pn << fb;
    packet.write((char*)&entries[0], entries.size()*SINT);

    idxfs.write(packet.getData(),packet.getSize());
}

void ExtendibleHash::readFromFile()
{
    idxfs.seekg(0,ios_base::beg);

    BufferPacket packet(SINT * 3);
    idxfs.read(packet.getData(), packet.getSize());
    
    packet >> gd >> pn >> fb;
    entries = vector<int> (1 << gd, 0);

    idxfs.read((char*)&entries[0], entries.size()*SINT);
}

bool ExtendibleHash::put(const Slice & key,const Slice & value)
{
    uint32_t hashVal = hashFunc(key);

    int cur     = hashVal & ((1 << gd) -1);
    /**Can use MemoryPool here**/
    int index = 0;
    Page * page = pcache -> find(entries.at(cur), index);
    bool entriesUpdate = false;

    if(page == NULL) 
    {
        page = new Page(this);
        index = pcache -> putInto(page, entries.at(cur));

        BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));

        datfs.seekg(entries.at(cur), ios_base::beg);
        datfs.read(packet.getData(), packet.getSize());

        page -> setByBucket(packet);
    }

    if(page -> full() && page -> d == gd)
    {
        log -> _Trace("ExtendibleHash :: put :: full, so double-entries\n");
        
        gd += 1; pn = 2*pn;
        
        int oldSize = entries.size();
        for(int i = 0; i < oldSize; i++)
            entries.push_back(entries.at(i));
        entriesUpdate = true;
        /**        
            writeToIdxFile();
        **/
    }

    if(page -> full() && page -> d < gd)
    {
        log -> _Trace("ExtendibleHash :: put :: split \n");
        if((page -> put(key, value, hashVal)) == 1)
        {
            //datfs.seekg(entries.at(cur), ios_base::beg);
            
           // BufferPacket packet = page -> getPacket();
           // datfs.write(packet.getData(), packet.getSize());
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

       /* for(index = curNum2;index < page -> curNum;index++)
            page -> elements[index].clear();*/
        
        page -> curNum = curNum2;
        p2   -> curNum = curNum3;
        datfs.seekg(0, ios_base::end);

        int oldpos = entries.at(cur);
        int oldpos2 = datfs.tellg();

        // assert(oldpos2 == datfileLen);
        
        for(index = 0; index < entries.size(); index++)
        {
            if(entries.at(index) == oldpos)
            {
                /**Problem ?**//**Must !!! **/
                if(((index >> (page -> d)) & 1) == 1)
                    entries[index] = oldpos2;
                else
                    entries[index] = oldpos;
            }
        }

        page -> d = p2 -> d = (page -> d) + 1;

       /* datfs.seekg(entries.at(cur),ios_base::beg);
        datfs.write((char*)page, sizeof(Page));

        datfs.seekg(0, ios_base::end);
        datfs.write((char*)p1, sizeof(Page));*/
/*        
        BufferPacket packe1 = page -> getPacket();
*/        
        BufferPacket packe2 = p2 -> getPacket();
        /**As I have move the pointer to the end of file, so just write**/
        datfs.write(packe2.getData(), packe2.getSize());
        datfileLen = oldpos2 + packe2.getSize();
        // assert(datfileLen == datfs.tellg());

        entriesUpdate = true;
        /*
            datfs.seekg(oldpos, ios_base::beg);
            datfs.write(packe1.getData(), packe1.getSize());
        */
       
        /**Must write p1 and p2 into files? bad style.**/

        /**Sometime it would write too much**/
        /**
           writeToIdxFile();
        **/
        /**must reset it at the end of this part**/
        /*
            pcache -> putInto(p1, oldpos);
            pcache -> putInto(p2, oldpos2);
        */
        /**Soooorry, bad style as I don't have good idea to hidden the delete method**/
        /*pcache -> specil();*/
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

    writeToIdxFile();
    
    return true;
}

Slice ExtendibleHash::get(const Slice & key)
{
    uint32_t hashVal = hashFunc(key);
    int cur = hashVal & ((1 << gd) -1);

    int index = 0;
    Page * page = pcache -> find(entries.at(cur), index);
    
    if(page == NULL) 
    {
        page = new Page(this);
        pcache -> putInto(page, entries.at(cur));

        BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));

        datfs.seekg(entries.at(cur), ios_base::beg);
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
    Page * page = pcache -> find(entries.at(cur), index);
    
    if(page == NULL) 
    {
        page = new Page(this);
        index = pcache -> putInto(page, entries.at(cur));

        BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));

        datfs.seekg(entries.at(cur), ios_base::beg);
        datfs.read(packet.getData(), packet.getSize());

        page -> setByBucket(packet);
    }

    int rb = page -> remove(key, hashVal);

    if(rb == 1)
    {
        BufferPacket npacket = page -> getPacket();
        
        datfs.seekg(entries.at(cur), ios_base::beg);
        datfs.write(npacket.getData(), npacket.getSize());

        pcache -> setUpdated(index);
    }
    else
        log -> _Trace("ExtendibleHash :: remove failed(maybe not exist?)");
    
    return rb;
}

int ExtendibleHash::findSuitableOffset(int size)
{
    EEmptyBlock block; int offset, pos;

    if(this -> fb == -1)
    {
        datfs.seekg(0, ios_base::end);
        
        this -> fb = datfs.tellg();

        block.eles[0].pos  = fb + SEEBLOCK;
        block.eles[0].size = size;
        block.curNum       = 1;

        datfs.write((char*)&block, SEEBLOCK);
        
        datfileLen = fb + SEEBLOCK + 2*size;

        writeToIdxFile();

        return fb + SEEBLOCK + size;
    }

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
            
            int index = 0; int bnum = block.curNum;
            
            block.nextBlock = nnBlock.nextBlock;

            datfs.seekg(0, ios_base::end);

            for(;index < nnBlock.curNum;index++)
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
            datfileLen = datfs.tellg();
        }
    }

    if(block.checkSuitable(size, pos) == true)
    {
        EEmptyBlock::EEmptyEle ele = block.eles[pos];

        offset = ele.pos; ele.pos += size;

        for(int index = pos + 1;index < block.curNum;index++)
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
            // assert(datfileLen + SEEBLOCK == datfs.tellg());
            datfileLen += SEEBLOCK;
        }

        datfs.seekg(2 * size,ios_base::end);
        
        offset = datfs.tellg(); offset -= size;

        block.eles[block.curNum].pos    = offset - size;
        block.eles[block.curNum++].size = size;
        
        datfileLen = datfs.tellg();
    }   

    datfs.seekg(fb, ios_base::beg);
    datfs.write((char*)&block, SEEBLOCK);

    return offset;
}

void ExtendibleHash::recycle(int offset, int size)
{
    EEmptyBlock block;

    if(fb == -1)
    {
        log -> _Fatal("There must be some fatal error\n");
        return;
    }

    /**Error?**/
    printf("notice5\n");
    
    {
        ScopeMutex scope(&(datLock));
        datfs.seekg(fb,      ios_base::beg);
        datfs.read((char*)&block, SEEBLOCK);
    }
    
    if(block.curNum == PAGESIZE)
    {
        ScopeMutex scope(&datLock);
        datfs.seekg(0, ios_base::end);

        int nn = block.nextBlock;
        
        EEmptyBlock nnBlock = block.split();
        nnBlock.nextBlock   = nn;
        block.nextBlock     = datfs.tellg();

        datfs.write((char*)&nnBlock, SEEBLOCK);
        datfileLen = datfs.tellg();
    }

    block.eles[block.curNum].pos    = offset;
    block.eles[block.curNum++].size = size;
    
    {
        ScopeMutex scope(&datLock);
        datfs.seekg(fb, ios_base::beg);
        datfs.write((char*)&block,SEEBLOCK);
    }
}

void ExtendibleHash::dump()
{
    pcache -> fflush();

    Page * page = new Page(this);
    cout << gd << " " << pn << " " << entries.size() << endl;
    for(int cur = 0, index = 0;cur < entries.size();cur++)
    {
        int j;
        for(j = 0;j < cur;j++)
        {
            if(entries.at(j) ==  entries.at(cur)) break;
        }

        if(j != cur) continue;

        BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));
        datfs.seekg(entries.at(cur), ios_base::beg);
        datfs.read(packet.getData(), packet.getSize());

        page -> setByBucket(packet);

        cout << "Page(size:"<<page -> curNum <<") " << index++ <<" "<<cur <<" "<<page -> d<<":";

        for(j = 0;j < page -> curNum;j++)
        {
            datfs.seekg(page -> elements[j].m_datPos, ios_base::beg);
            BufferPacket packet(2*SINT);
            datfs.read(packet.getData(),packet.getSize());
            int a, b; uint32_t hashVal = page -> elements[j].m_hashVal;
            packet >> a >> b;
            cout << a <<" " << b << " "<<hashVal<<" ";
        }

        cout << endl;
    }

    delete page; page = NULL;
}

void ExtendibleHash::printThisPage(Page * page)
{
    for(int j = 0;j < page -> curNum;j++)
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
    datfs.seekg(0, ios_base::end);
    uint32_t curpos = datfs.tellg();

    BufferPacket phyPacket(pbatch->getTotalSize());
    datfs.write(phyPacket.getData(), phyPacket.getSize());

    uint32_t totalSize = 0;
    WriteBatch::Iterator iterator(pbatch);

    for(const Node * node = iterator.first();node != iterator.end();node = iterator.next())
    {
        Slice key   = node -> first;
        Slice value = node -> second;
                
        uint32_t hashVal = hashFunc(key);
        int cur   = hashVal & ((1 << gd) -1);
        int index = 0;

        Page * page = pcache -> find(entries.at(cur), index);

        if(page == NULL) 
        {
            page  = new Page(this);
            index = pcache -> putInto(page, entries.at(cur));

            BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));

            datfs.seekg(entries.at(cur), ios_base::beg);
            datfs.read(packet.getData(), packet.getSize());

            page -> setByBucket(packet);
        }

        if(page -> full() && page -> d == gd)
        {
            gd++; pn = 2*pn;
            int oldSize = entries.size();
            for(int i = 0; i < oldSize; i++)
                entries.push_back(entries.at(i));
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
            
            /**Can focus on whether it can be reduced to zero**/
            datfs.seekg(0, ios_base::end);

            int oldpos = entries.at(cur);
            int oldpos2 = datfs.tellg();
            /**what's wrong with this line**/
            /*assert(datfileLen == oldpos2);*/
            for(index = 0; index < entries.size(); index++)
            {
                if(entries.at(index) == oldpos)
                {
                    if(((index >> (page -> d)) & 1) == 1)
                        entries[index] = oldpos2;
                    else
                        entries[index] = oldpos;
                }
            }

            page -> d = p2 -> d = (page -> d) + 1;
      
            BufferPacket packe2 = p2 -> getPacket();
            datfs.write(packe2.getData(), packe2.getSize());
            datfileLen = datfs.tellg();
        }
        else
        {
            page -> replaceQ(key, value, hashVal, totalSize + curpos);
            phyPacket << key << value;
            totalSize = totalSize + key.size() + value.size();
           
            pcache -> setUpdated(index);
        }
    }

    datfs.seekg(curpos, ios_base::beg);
    datfs.write(phyPacket.getData(), phyPacket.getSize());
    
    writeToIdxFile();
    pcache -> free();
}

struct ExtendibleHash::Writer {
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
    while (!w.done && &w != m_writers.front()) { w.cv.wait(); }
    if (w.done) { return; }
    
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

    if (!m_writers.empty()) {
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

    FILE * tmpfile1 = fopen("tmppage.bak","wb");
    FILE * tmpfile2 = fopen("tmpcon.bak","wb");
    assert(tmpfile1 && tmpfile2);
    
    WriteBatch * pbatch =  new WriteBatch(PAGESIZE*2);

    for(int cur = 0;cur < entries.size();cur++)
    {
        int j;
        for(j = 0;j < cur;j++)
        {
            if(entries.at(j) ==  entries.at(cur)) break;
        }

        if(j != cur) continue;

        BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));
        datfs.seekg(entries.at(cur), ios_base::beg);
        datfs.read(packet.getData(), packet.getSize());

        page -> setByBucket(packet);

        for(j = 0;j < page -> curNum;j++)
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
        for(const Node * node = iter.first();node != iter.end();node = iter.next())
        {
            packet1 << (node -> first) << (node -> second);
        }
        uint32_t size = packet1.getSize();

        fwrite((char*)&(size), sizeof(uint32_t), 1, tmpfile2);
        nmeb = fwrite(packet1.getData(), packet1.getSize(), 1, tmpfile2);
        assert(nmeb == 1);

        pbatch -> clear();
    }

    idxfs.close(); datfs.close();

    string nidxName = "rm " + idxName;
    string ndatName = "rm " + datName;
    system(nidxName.c_str());
    system(ndatName.c_str());

     /**Sorry for that, I don't find better solution.**/
    idxfs.open (idxName.c_str(), FILEMODE2);
    datfs.open (datName.c_str(), FILEMODE2);
    
    idxfs.close(); datfs.close();

    idxfs.open (idxName.c_str(), FILEMODE1);
    datfs.open (datName.c_str(), FILEMODE1);

    vector<char> used(entries.size(), 0);

        fseek(tmpfile1,0,SEEK_SET);
    fseek(tmpfile2,0,SEEK_SET);

    fclose(tmpfile1); fclose(tmpfile2);
    tmpfile1 = fopen("tmppage.bak","rb");
    tmpfile2 = fopen("tmpcon.bak","rb");
    assert(tmpfile1 && tmpfile2);
    
    fseek(tmpfile1, 0, SEEK_SET);
    fseek(tmpfile2, 0, SEEK_SET);

    uint32_t uds = 0;
    for(int cur = 0;cur < entries.size();cur++)
    {
        if(used.at(cur) == 0)
        {
            vector<int> ids; ids.push_back(cur);

            for(int j = cur + 1;j < entries.size();j++)
            {
                if(entries.at(cur) == entries.at(j))
                {    
                    used[j] = 1;
                    ids.push_back(j);
                }
            }

            for(int j=0;j<ids.size();j++)
            {
                entries[ids.at(j)] = uds;
            }

            BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));
            int nmeb = fread(packet.getData(), packet.getSize(), 1, tmpfile1);
            cout<<packet.getSize()<<" "<<GetFileLen(tmpfile1)<<" "<<ftell(tmpfile1)<<endl;
            assert(nmeb == 1);

            page -> setByBucket(packet);

            uint32_t size;
            nmeb = fread((char*)&size, sizeof(uint32_t), 1, tmpfile2);
            assert(nmeb == 1);

            BufferPacket packet1(size);
            nmeb = fread(packet1.getData(), packet1.getSize(), 1, tmpfile2);
            assert(nmeb == 1);

            int fpos = uds + packet.getSize();
            for(int j=0;j<page->curNum;j++)
            {
                page->elements[j].m_datPos = fpos;
                fpos += page->elements[j].m_keySize + page->elements[j].m_datSize;
            }

            packet = page->getPacket();
            cout<< page->curNum <<" "<<packet.getSize() << " " << packet1.getSize() << endl;
            datfs.write(packet.getData(), packet.getSize());
            datfs.write(packet1.getData(), packet1.getSize());

            uds += packet.getSize() + packet1.getSize();
        }
    }

    this -> fb = -1;
    datfs.flush();
    writeToIdxFile();
    idxfs.flush();

    delete page; page = NULL;
    delete pbatch; pbatch = NULL;
}

void ExtendibleHash::runBatch2(const WriteBatch * pbatch)
{
    BufferPacket phyPacket(pbatch->getTotalSize());

    uint32_t curpos = 0, totalSize = 0; 
    Page * page;

    {
        ScopeMutex scope(&datLock);
        datfs.seekg(0, ios_base::end);
        curpos = datfs.tellg();
        datfs.write(phyPacket.getData(), phyPacket.getSize());
    }    

    WriteBatch::Iterator iterator(pbatch);

    for(const Node * node = iterator.first();node != iterator.end();node = iterator.next())
    {
        Slice key   = node -> first;
        Slice value = node -> second;

       // cout<< key.returnAsInt() << endl;
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
           // printf("oh my.god\n");
            globalLock.writeLock();
        }

        int cur   = hashVal & ((1 << gd) -1);
        assert((1<<gd) <= entries.size());
        int index = 0;

        page = NULL;

        {
            cacheLock.lock();
            assert(cur < entries.size());
            page = pcache -> find(entries.at(cur), index);

            if(page != NULL)
            {
                if(cacheElemLock[index].trylock()==0)
                {
                    cacheLock.unlock();
                }
                else
                {
                 //   printf("notice\n");
                    cacheLock.unlock();
                    cacheElemLock[index].lock();
                }
            }
            else
                cacheLock.unlock();
        }

        if(page != NULL)
        {
            if(page == pcache -> cacheElems[index].page && entries.at(cur) == pcache -> cacheElems[index].entry)
            {
            }
            else
            {
                printf("notice2\n");
                cacheElemLock[index].unlock();
                if(globalFlag == 0)
                    globalLock.readUnlock();
                else
                    globalLock.writeUnlock();
                goto LABLE;
            }
        }
        else if(page == NULL) 
        {
            bool flag1 = true; page  = new Page(this);

            index = -1;
            while(flag1)
            {                
                {
                    ScopeMutex lock(&cacheLock);
                    index = pcache -> findLockable(page, entries.at(cur));
                }

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
                assert(cur < entries.size());
                datfs.seekg(entries.at(cur), ios_base::beg);
                datfs.read(packet.getData(), packet.getSize());
            }

            page -> setByBucket(packet);
        }

        int index2 = index;
        /**could use advanced optimization**/
        if(page -> full())
        {
            if(globalFlag == 0)
            {
                globalLock.readUnlock();
                cacheElemLock[index].unlock();
                globalFlag = 1;
                goto LABLE;
            }

            if(page -> getD() == gd)
            {   
                gd++; pn = 2*pn;
                int oldSize = entries.size();
                for(int i = 0; i < oldSize; i++)
                    entries.push_back(entries.at(i));
            }

            page -> replaceQ(key, value, hashVal, totalSize + curpos);
            phyPacket << key << value;
            totalSize += key.size() + value.size();

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
            
            int oldpos2;

            p2   -> setD(page->getD() + 1);
            page -> setD(p2->getD());
            BufferPacket packe2 = p2 -> getPacket();

            {
                ScopeMutex scope(&datLock);
                datfs.seekg(0, ios_base::end);
                oldpos2 = datfs.tellg();
                datfs.write(packe2.getData(), packe2.getSize());
            }

            assert(cur < entries.size());
            int oldpos = entries.at(cur);
            
            /**could be further optimization, but should consider the frequency**/
            for(index = 0; index < entries.size(); index++)
            {
                if(entries.at(index) == oldpos)
                {
                    if(((index >> (page -> d)) & 1) == 1)
                        entries[index] = oldpos2;
                    else
                        entries[index] = oldpos;
                }
            }

            delete p2; p2 = NULL;
        }
        else
        {
            page -> replaceQ(key, value, hashVal, totalSize + curpos);
            
            phyPacket << key << value;
            totalSize += key.size() + value.size();           
        }       

        pcache -> setUpdated(index2);

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
        datfs.seekg(curpos, ios_base::beg);
        datfs.write(phyPacket.getData(), phyPacket.getSize());
    }
}
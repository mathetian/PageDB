#include "FactoryImpl.h"

#define FILEMODE1 (fstream::in | fstream::out)

#define FILEMODE2 (fstream::out | fstream::app)

static uint32_t datfileLen = 0;

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

    assert(eHash -> datfs.tellg() == datfileLen);

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

            eHash -> datfs.seekg(element.m_datPos, ios_base::beg);
            eHash -> datfs.read(packet.getData(), packet.getSize());
            
            packet.setBeg(); 
            packet >> slice;
            
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
        hashFunc(hashFunc), gd(0), pn(1), fb(-1) { pcache = new PageCache(this);}

ExtendibleHash::~ExtendibleHash()
{ 
    if(idxfs) writeToIdxFile();
    if(idxfs) idxfs.close(); 
    if(datfs) datfs.close();
    if(pcache) delete pcache;
}

void ExtendibleHash::fflush()
{
    pcache -> free();
    idxfs.flush();
    datfs.flush();
}

bool ExtendibleHash::init(const char * filename)
{
    struct stat buf;

    string sfilename(filename, filename + strlen(filename));
    string idxName = sfilename + ".idx";
    string datName = sfilename +  ".dat";

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

        assert(oldpos2 == datfileLen);
        
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
        assert(datfileLen == datfs.tellg());

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
            assert(datfileLen + SEEBLOCK == datfs.tellg());
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
        datfileLen = datfs.tellg();
    }

    block.eles[block.curNum].pos    = offset;
    block.eles[block.curNum++].size = size;

    datfs.seekg(fb, ios_base::beg);
    datfs.write((char*)&block,SEEBLOCK);
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

void ExtendibleHash::runBatch(const WriteBatch & batch)
{
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
        assert(datfileLen + 2*SEEBLOCK == datfs.tellg());
        datfileLen += 2*SEEBLOCK;
    }

    datfs.seekg(0, ios_base::end);
    uint32_t curpos = datfs.tellg();
    assert(curpos == datfileLen);
    BufferPacket phyPacket(batch.getTotalSize());
    datfs.write(phyPacket.getData(), phyPacket.getSize());

    uint32_t totalSize = 0;
    typedef pair<Slice, Slice> Node;    
    WriteBatch::Iterator iterator(&batch);

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

void ExtendibleHash::write(WriteBatch * my_batch)
{
    if(my_batch == NULL) return;

    Writer w(&m_mutex);
    w.batch = my_batch;
    w.sync = options.sync;
    w.done = false;

    ScopeMutex l(&m_mutex);
    m_writers.push_back(&w);
    while (!w.done && &w != m_writers.front()) { w.cv.Wait(); }
    if (w.done) { return; }
    
    Writer* last_writer = &w;
    WriteBatch* updates = BuildBatchGroup(&last_writer);
    
    {
        m_mutex.unlock();
        /**When it is directed to here, only one thread can access it at any time.**/
        runBatch(*updates);
        m_mutex.lock();
    }

    while (true) {
        Writer* ready = m_writers.front();
        m_writers.pop_front();
        if (ready != &w) {
            ready->done = true;
            ready->cv.Signal();
        }
        if (ready == last_writer) break;
    }

    if (!m_writers.empty()) {
        m_writers.front()->cv.Signal();
    }
}

WriteBatch* BuildBatchGroup(WriteBatch ** ppbatch)
{
    Writer* first = writers_.front();
    WriteBatch* result = first->batch;

  size_t size = WriteBatchInternal::ByteSize(first->batch);

  // Allow the group to grow up to a maximum size, but if the
  // original write is small, limit the growth so we do not slow
  // down the small write too much.
  size_t max_size = 1 << 20;
  if (size <= (128<<10)) {
    max_size = size + (128<<10);
  }

  *last_writer = first;
  std::deque<Writer*>::iterator iter = writers_.begin();
  ++iter;  // Advance past "first"
  for (; iter != writers_.end(); ++iter) {
    Writer* w = *iter;
    if (w->sync && !first->sync) {
      // Do not include a sync write into a batch handled by a non-sync write.
      break;
    }

    if (w->batch != NULL) {
      size += WriteBatchInternal::ByteSize(w->batch);
      if (size > max_size) {
        // Do not make batch too big
        break;
      }

      // Append to *reuslt
      if (result == first->batch) {
        // Switch to temporary batch instead of disturbing caller's batch
        result = tmp_batch_;
        assert(WriteBatchInternal::Count(result) == 0);
        WriteBatchInternal::Append(result, first->batch);
      }
      WriteBatchInternal::Append(result, w->batch);
    }
    *last_writer = w;
  }
  return result;
}
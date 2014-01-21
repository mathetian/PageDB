#include "FactoryImpl.h"

#define FILEMODE1 (fstream::in | fstream::out)

#define FILEMODE2 (fstream::out | fstream::app)

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

bool Page::put(const Slice & key, const Slice & value, int hashVal)
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

    /**
        Modify the page index
    **/

    elements[curNum].m_hashVal   = hashVal;
    elements[curNum].m_datPos    = offset;
    elements[curNum].m_keySize   = key.size();
    elements[curNum++].m_datSize = value.size();

    return 1;
}

Slice Page::get(const Slice & key, int hashVal)
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

bool Page::remove(const Slice & key, int hashVal)
{
    int index, rindex;
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
        BufferPacket packet = page -> getPacket();
        datfs.write(packet.getData(),packet.getSize());

        delete page; page = NULL;
    }
    else 
        readFromFile();
    return true;
}

void ExtendibleHash::writeToIdxFile()
{
    idxfs.seekg(0,ios_base::beg);
    BufferPacket packet(SINT * 3 + entries.size()*SINT);
    packet << gd << pn << fb;

    packet.write((char*)&entries[0], entries.size()*SINT);

    idxfs.write(packet.getData(),packet.getSize());

    idxfs.flush();
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
    int hashVal = hashFunc(key);
    int cur     = hashVal & ((1 << gd) -1);

    /**Can use MemoryPool here**/
    Page * page = new Page(this);

    BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));
    
    datfs.seekg(entries.at(cur), ios_base::beg);
    datfs.read(packet.getData(), packet.getSize());

    page -> setByBucket(packet);
    
    if(page -> full() && page -> d == gd)
    {
        log -> _Trace("ExtendibleHash :: put :: full, so double-entries\n");
        this -> gd++; 
        int oldSize = entries.size();
        for(int i = 0; i < oldSize; i++)
            entries.push_back(entries.at(i));
        writeToIdxFile();
        cout << "double-entries" << endl;
    }

    if(page -> full() && page -> d < gd)
    {
        log -> _Trace("ExtendibleHash :: put :: split \n");
        cout << "put::split" << endl;
       // dump();

        if((page -> put(key, value, hashVal)) == 1)
        {
            //datfs.seekg(entries.at(cur), ios_base::beg);
            
           // BufferPacket packet = page -> getPacket();
           // datfs.write(packet.getData(), packet.getSize());
        }
        else
        {
            log -> _Warn("ExtendibleHash :: put :: HAS been in the file\n");
            delete page; page = NULL; return false;
        }

        Page * p1 = new Page(this);
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
                p1 -> elements[curNum2++] = page -> elements[index];
        }

       /* for(index = curNum2;index < page -> curNum;index++)
            page -> elements[index].clear();*/
        
        p1   -> curNum = curNum2;
        p2   -> curNum = curNum3;
        cout << "curNUm: "<<curNum2 <<" "<<curNum3<<endl;
        datfs.seekg(0, ios_base::end);

        int oldpos = entries.at(cur);
        for(index = 0; index < entries.size(); index++)
        {
            if(entries.at(index) == entries.at(cur))
            {
                /**Problem ?**//**Must !!! **/
                cout << index << " "<<((index >> (page -> d)) & 1)<<endl;
                if(((index >> (page -> d)) & 1) == 1)
                    entries[index] = datfs.tellg();
                else
                    entries[index] = oldpos;
            }
        }

        p1 -> d = p2 -> d = (page -> d) + 1;

       /* datfs.seekg(entries.at(cur),ios_base::beg);
        datfs.write((char*)page, sizeof(Page));

        datfs.seekg(0, ios_base::end);
        datfs.write((char*)p1, sizeof(Page));*/
        BufferPacket packe1 = p1 -> getPacket();
        BufferPacket packe2 = p2 -> getPacket();
        cout << cur <<endl;
        datfs.seekg(oldpos, ios_base::beg);
        datfs.write(packe1.getData(), packe1.getSize());

        datfs.seekg(0, ios_base::end);
        datfs.write(packe2.getData(), packe2.getSize());
        /**Sometime it would write too much**/
       
        writeToIdxFile();

        delete p1; p1 = NULL; delete p2; p2 = NULL;

       // cout << "After Split" << endl;
      //  dump();
    }
    else
    {
        if((page -> put(key, value, hashVal)) == 1)
        {
            BufferPacket packet = page -> getPacket();

            datfs.seekg(entries.at(cur),  ios_base::beg); 
            datfs.write(packet.getData(), packet.getSize());
        }
        else
        {
            delete page; page = NULL;
            log -> _Warn("ExtendibleHash :: put :: HAS been in the file\n");
            return false;
        }
    }
        
    delete page; page = NULL;

    return true;
}

Slice ExtendibleHash::get(const Slice & key)
{
    int vvvv = key.returnAsInt();
    
    int hashVal = hashFunc(key);
    int cur     = hashVal & ((1 << gd) -1);

    Page * page = new Page(this);
    BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));
    
    datfs.seekg(entries.at(cur), ios_base::beg);
    datfs.read(packet.getData(), packet.getSize());

    page -> setByBucket(packet);

   /* printThisPage(page);*/

    Slice rs = page -> get(key, hashVal);

    delete page;
    page = NULL;

    if(vvvv == 32)
        rs.printAsInt();

    return rs;
}

bool ExtendibleHash::remove(const Slice & key)
{
    int hashVal = hashFunc(key);
    int cur     = hashVal & ((1 << gd) -1);
    
    Page * page = new Page(this);
    BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));
    
    datfs.seekg(entries.at(cur), ios_base::beg);
    datfs.read(packet.getData(), packet.getSize());

    page -> setByBucket(packet);

    int rb = page -> remove(key, hashVal);

    if(rb == 1)
    {
        BufferPacket npacket = page -> getPacket();
        
        datfs.seekg(entries.at(cur), ios_base::beg);
        datfs.write(npacket.getData(), npacket.getSize());
    }
    else
        log -> _Trace("ExtendibleHash :: remove failed(maybe not exist?");

    delete page;
    page = NULL;
    
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

        idxfs.seekg(2 * SINT, ios_base::beg);
        idxfs.write((char*)&fb, SINT);
        

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
        }

        datfs.seekg(2 * size,ios_base::end);
        
        offset = datfs.tellg(); offset -= size;

        block.eles[block.curNum].pos    = offset - size;
        block.eles[block.curNum++].size = size;
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
    }

    block.eles[block.curNum].pos    = offset;
    block.eles[block.curNum++].size = size;

    datfs.seekg(fb, ios_base::beg);
    datfs.write((char*)&block,SEEBLOCK);
}

void ExtendibleHash::dump()
{
    Page * page = new Page(this);

    for(int cur = 0;cur < entries.size();cur++)
    {
        int j;
        for(j = 0;j < cur;j++)
        {
            if(entries[j] ==  entries[cur]) break;
        }
        if(j != cur) continue;

        BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));
        datfs.seekg(entries.at(cur), ios_base::beg);
        datfs.read(packet.getData(), packet.getSize());

        page -> setByBucket(packet);

        cout << "Page(size:"<<page -> curNum <<") " << cur <<":";

        for(j = 0;j < page -> curNum;j++)
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
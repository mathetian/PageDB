#include <sys/stat.h>
#include <string.h>

#include "../include/EHash.h"
#include "../helpers/BufferPacket.h"

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
    
    packet.read((char*)&elements[0],SPELEMENT*(PAGESIZE + 5));
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
            BufferPacket packet(elements[index].m_datSize);
            Slice slice(elements[index].m_datSize);

            eHash -> datfs.seekg(elements[index].m_datPos + elements[index].m_keySize, ios_base::beg);
            eHash -> datfs.read(packet.getData(), packet.getSize());
            
            packet >> slice;
            return slice;
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

ExtendibleHash::ExtendibleHash(HASH hashFunc)
{
    this -> hashFunc = hashFunc;
    this -> gd = 0;
    this -> pn = 1;
    this -> fb = -1;
}

ExtendibleHash::~ExtendibleHash()
{
    if(page) delete page;
    page = NULL;
    if(idxfs) idxfs.close();
    if(datfs) datfs.close();
}

bool ExtendibleHash::init(const string&filename)
{
    struct stat buf;
    string idxName = filename + ".idx";
    string datName = filename +  ".dat";

    idxfs.open (idxName.c_str(), FILEMODE2);
    datfs.open (datName.c_str(), FILEMODE2);
    
    idxfs.close(); datfs.close();

    idxfs.open (idxName.c_str(), FILEMODE1);
    datfs.open (datName.c_str(), FILEMODE1);
    if(!idxfs || !datfs)
    {
        printf("ExtendibleHash::init::open error\n");
        return false;
    }

    if((stat(idxName.c_str(), &buf) == -1) || buf.st_size == 0)
    {
        gd = 0;
        pn = 1;
        entries.push_back(0);

        page = new Page(this);

        idxfs.seekg(0,ios_base::beg);
        writeToFile();

        datfs.seekg(0,ios_base::beg);
        BufferPacket packet = page -> getPacket();
        datfs.write(packet.getData(),packet.getSize());
        datfs.flush();

        delete page;
        page = NULL;
    }
    else readFromFile();
    return true;
}

bool ExtendibleHash::put(const string&key,const string&value)
{
    int hashVal = hashFunc(key);
    int cur     = hashVal & ((1 << gd) -1);

    page        = new Page(this);

    BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));
    
    datfs.seekg(entries.at(cur), ios_base::beg);
    datfs.read(packet.getData(), packet.getSize());

    page -> setBucket(packet);
    
    if(page -> full() && page -> d == gd)
    {

        this -> gd++; 
        int oldSize = entries.size();
        for(int i = 0; i < oldSize; i++)
            entries.push_back(entries.at(i));
        writeToFile();
    }

    if(page -> full() && page -> d < gd)
    {
        log -> _Trace("ExtendibleHash :: put :: split \n");

        if((page -> put(key, value, hashVal)) == 1)
        {
            datfs.seekg(entries.at(cur), ios_base::beg);
            BufferPacket packet = page -> getPacket();
            datfs.write(packet.getData(), packet.getSize());
        }
        else
        {
            log -> _Warn("ExtendibleHash :: put :: HAS been in the file\n");
            delete page; page = NULL; return false;
        }

        Page * p1 = new Page(this);

        int index = 0;
        int curNum2 = 0, curNum3 = 0;

        for(index = 0; index < page -> curNum; index++)
        {
            PageElement element = page -> elements[index];
            datfs.seekg(element.data_pointer,ios_base::cur);
            
            BufferPacket packet(element.key_size);
            datfs.read(packet.getData(),packet.getSize());
            string str(packet.getSize(),0);
            packet >> str;

            int id = hashFunc(str);
            int flag = (id & ((1 << gd) - 1));
            if(((flag >> (page -> d)) & 1) == 1)
                p1 -> elements[curNum3++] = page -> elements[index];
            else
                page -> elements[curNum2++] = page -> elements[index];
        }

        page -> curNum = curNum2;
        p1 -> curNum = curNum3;

        for(index = 0; index < entries.size(); index++)
        {
            if(entries.at(index) == entries.at(cur))
            {
                if(((index >> (page -> d)) & 1) == 1)
                    entries[index] = datfs.tellg();
            }
        }
        page -> d = p1 -> d = (page -> d) + 1;
        datfs.seekg(entries.at(cur),ios_base::beg);
        datfs.write((char*)page, sizeof(Page));

        datfs.seekg(0,ios_base::end);
        datfs.write((char*)p1, sizeof(Page));

        writeToFile();

        delete p1; p1 = NULL;
    }
    else
    {
        if((page -> put(key, value, hashVal)) == 1)
        {
            BufferPacket packet = page -> getPacket();
            datfs.seekg(0, ios_base::beg); 
            datfs.write(packet.getData(), packet.getSize());
            datfs.flush();
        }
        else
        {
            delete page;
            page = NULL;
            log -> _Warn("ExtendibleHash :: put :: HAS been in the file\n");
            return false;
        }
    }
        
    delete page;
    page = NULL;

    return true;
}

string ExtendibleHash::get(const string&key)
{
    int hashVal = hashFunc(key);
    int cur     = hashVal & ((1 << gd) -1);
    
    page        = new Page(this);

    
    BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));
    
    datfs.seekg(entries.at(cur), ios_base::beg);
    datfs.read(packet.getData(), packet.getSize());

    page -> setBucket(packet);

    string rs = page -> get(key, hashVal);

    delete page;
    page = NULL;

    return rs;
}

bool ExtendibleHash::remove(const string&key)
{
    int hashVal = hashFunc(key);
    int cur     = hashVal & ((1 << gd) -1);
    page        = new Page(this);
    
    BufferPacket packet(2*SINT + SPELEMENT*(PAGESIZE + 5));
    
    datfs.seekg(entries.at(cur), ios_base::beg);
    datfs.read(packet.getData(), packet.getSize());

    page -> setBucket(packet);

    int rb = page -> remove(key, hashVal);

    if(rb == 1)
    {
        BufferPacket npacket(2*SINT + SPELEMENT*(PAGESIZE + 5));
        datfs.seekg(entries.at(cur), ios_base::beg);
        datfs.write(npacket.getData(), npacket.getSize());
    }

    delete page;
    page = NULL;
    
    return rb;
}

void ExtendibleHash::writeToFile()
{
    BufferPacket packet(SINT * 3 + entries.size()*SINT);
    packet << gd << pn << fb;

    packet.write((char*)&entries[0], entries.size()*SINT);

    idxfs.write(packet.getData(),packet.getSize());
    idxfs.flush();
}

void ExtendibleHash::readFromFile()
{
    BufferPacket packet(SINT * 3);
    
    idxfs.read(packet.getData(), packet.getSize());
    
    packet >> gd >> pn >> fb;

    entries = vector<int> (1 << gd, 0);

    idxfs.read((char*)&entries[0], entries.size()*SINT);
}

int ExtendibleHash::findSuitable(int size)
{
    EEmptyBlock block; int offset, pos;

    if(fb == -1)
    {
        
        datfs.seekg(0, ios_base::end);
        this -> fb = datfs.tellg();

        block.eles[0].pos  = fb + SEEBLOCK;
        block.eles[0].size = size;
        block.curNum       = 1;

        datfs.write((char*)&block, SEEBLOCK);

        datfs.flush();

        idxfs.seekg(2 * SINT, ios_base::beg);
        idxfs.write((char*)&fb, SINT);
        idxfs.flush();

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

            datfs.seekg(0,ios_base::end);

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

                block.eles[block.curNum].pos    = old;
                block.eles[block.curNum++].size = SEEBLOCK;
            }
        }
    }

    if(block.checkSuitable(size, pos) == true)
    {
        EEmptyEle ele = block.eles[pos];
        
        offset = ele.pos;
        
        ele.pos += size;

        int index = pos + 1;

        for(;index < block.curNum;index++)
            block.eles[index - 1] = block.eles[index];

        if(block.eles[pos].size == size) block.curNum--;
        else block.eles[block.curNum - 1] = ele;
    }
    else
    {
        if(block.curNum == PAGESIZE)
        {
            datfs.seekg(0,ios_base::end);
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

    datfs.flush();

    return offset;
}

int defaultHashFunc(const string&str)
{
    int index;
    int value = 0x238F13AF * str.size();
    for(index = 0; index < str.size(); index++)
        value = (value + (str.at(index) << (index*5 % 24))) & 0x7FFFFFFF;
    return value;
}

void ExtendibleHash::cycle(int offset, int size)
{
    EEmptyBlock block;

    if(fb == -1)
        log -> _Error("There must be some fatal error\n");

    datfs.seekg(fb,    ios_base::beg);
    datfs.read((char*)&block, SEEBLOCK);
    
    if(block.curNum == PAGESIZE)
    {
        int nn = block.nextBlock;
        EEmptyBlock nnBlock = block.split();
        nnBlock.nextBlock   = nn;
        
        datfs.seekg(0, ios_base::end);
        block.nextBlock     = datfs.tellg();

        datfs.write((char*)&nnBlock, SEEBLOCK);
        datfs.flush();
    }

    block.eles[block.curNum].pos    = offset;
    block.eles[block.curNum++].size = size;

    datfs.seekg(fb, ios_base::beg);
    datfs.write((char*)&block,SEEBLOCK);
    datfs.flush();
}
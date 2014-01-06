#include <sys/stat.h>
#include <string.h>

#include "EHash.h"
#include "BufferPacket.h"

#define FILEMODE (std::fstream::in | std::fstream::out | std::fstream::app)

bool EEmptyBlock::checkSuitable(int size, int & pos)
{
    for(pos = curNum - 1; pos >= 0; pos--)
    {
        if(eles[pos].size > size)
            return true;
    }
    return false;
}

BufferPacket Page::getPacket()
{
    BufferPacket packet(SINT * 2 + sizeof(elements));
    packet << d << curNum;
    packet.write((char*)&elements[0], sizeof(elements));
    return packet;
}

void  Page::setBucket(BufferPacket & packet)
{
    packet.setBeg();
    packet >> d >> curNum;
    packet.read((char*)&elements[0],SPELEMENT*(PAGESIZE + 5));
}

bool Page::put(const string&key, const string&value, int hashVal)
{
    int index;
    for(index = 0; index < PAGESIZE + 5; index++)
    {
        PageElement element = elements[index];
        if(element.hash_value != -1 && element.hash_value == hashVal)
        {
            if(element.key_size == key.size() && element.data_size == value.size())
            {
                /**
                  Read From File, check it whether double
                **/
                eHash -> datfs.seekg(element.data_pointer,ios_base::beg);
                BufferPacket packet(element.key_size + element.data_size);
                eHash -> datfs.read(packet.getData(), packet.getSize());
                packet.setBeg();string str1(packet.getSize(),0);
                packet >> str1;
                if(str1 == key) return 0;
            }
        }
    }
    /**
      Find an suitable empty block, if not allocated it at the end of the file
    **/
    int offset = eHash -> findSuitable(key.size() + value.size());
    eHash -> datfs.seekg(offset, ios_base::beg);
    eHash -> datfs.write(key.c_str(), sizeof(key));
    eHash -> datfs.write(value.c_str(), sizeof(value));

    return 1;
}

string Page::get(const string&key, int hashVal)
{
    int index;
    for(index = 0; index < curNum; index++)
    {
        if(elements[index].hash_value == hashVal && elements[index].key_size == key.size())
        {
            eHash -> datfs.seekg(elements[index].data_pointer + elements[index].key_size, ios_base::beg);
            BufferPacket packet(elements[index].data_size);
            eHash -> datfs.read(packet.getData(), packet.getSize());
            string value(packet.getSize(),0);
            packet >> value;
            return value;
        }
    }
    return "";
}

bool Page::remove(const string&key, int hashVal)
{
    int index, rindex;
    for(index = 0; index < curNum; index++)
    {
        if(elements[index].hash_value == hashVal && elements[index].key_size == key.size())
        {
            eHash -> datfs.seekg(elements[index].data_pointer, ios_base::beg);
            BufferPacket packet(elements[index].key_size);
            eHash -> datfs.read(packet.getData(), packet.getSize());
            if(strcmp(packet.getData(), key.c_str()) == 0) break;
        }
    }
    if(index == curNum) return false;

    /**
      Attach the space to the emptryBlock
    **/
    eHash -> cycle(elements[index].data_pointer, elements[index].key_size + elements[index].data_size);

    for(; index < curNum - 1; index++)
        elements[rindex] = elements[rindex + 1];
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

    idxfs.open (idxName.c_str(), FILEMODE);
    datfs.open (datName.c_str(), FILEMODE);
    
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
    page -> printAllEle();
    
    if(page -> full() && page -> d == gd)
    {
        this -> gd++; 
        int oldSize = entries.size();
        for(int i = 0; i < oldSize; i++)
            entries.push_back(entries.at(i));

        idxfs.seekg(0,ios_base::end);
        idxfs.write((char*)&entries[oldSize], oldSize*SINT);
    }

    if(page -> full() && page -> d < gd)
    {
        log -> _Trace("ExtendibleHash :: put :: split \n");

        page -> put(key, value, hashVal);

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
    }
    else
        page -> put(key, value, hashVal);
        
    delete page;
    page = NULL;
    
    return true;
}

string ExtendibleHash::get(const string&key)
{
    int hashVal = hashFunc(key);
    int cur     = hashVal & ((1 << gd) -1);
    
    page        = new Page(this);

    datfs.seekg(entries.at(cur), ios_base::beg);
    datfs.read((char*)page, SPAGE);

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
    datfs.seekg(entries.at(cur), ios_base::beg);
    datfs.read((char*)page, SPAGE);

    int rb = page -> remove(key, hashVal);

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
    EEmptyBlock block;
    int offset, pos;
    int blockDir;
    if(fb == -1)
    {
        block.curNum = 1;
        block.eles[0].pos = datfs.tellg();
        block.eles[0].size = size;
        
        offset  = datfs.tellg();
        offset += size;
        
        datfs.seekg(2*size,ios_base::end);
        datfs.write((char*)&block, SEEBLOCK);
        
        datfs.flush();

        return offset;
    }

    datfs.seekg(fb, ios_base::cur);
    datfs.write((char*)&block, SEEBLOCK);

    blockDir = fb;
    while(block.checkSuitable(size, pos) == false)
    {
        if(block.nextBlock == -1) break;
        blockDir = block.nextBlock;
        idxfs.seekg(block.nextBlock, ios_base::beg);
        idxfs.read((char*)&block, SEEBLOCK);
    }

    if(block.nextBlock == -1)
    {
        EEmptyBlock nBlock;
        nBlock.curNum = 1;
        block.eles[0].pos = datfs.tellg();
        block.eles[0].size = size;
        offset = datfs.tellg();
        offset += size;
        datfs.seekg(2 * size, ios_base::end);
        datfs.write((char*)&block, SEEBLOCK);
    }
    else
    {
        if(block.eles[pos].size == size)
        {
            block.curNum --;
            offset = block.eles[pos].pos;
            if(block.curNum == 0)
            {
                block.curNum ++;
                block.eles[0].pos = datfs.tellg();
                block.eles[0].size = size;
            }
            datfs.seekg(blockDir, ios_base::beg);
            datfs.write((char*)&block, SEEBLOCK);
        }
        else
        {
            offset = block.eles[pos].pos;
            block.eles[pos].size -= size;
            datfs.seekg(blockDir, ios_base::beg);
            datfs.write((char*)&block, SEEBLOCK);
        }
    }
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
    {
        block.curNum = 1;
        block.eles[0].pos = offset;
        block.eles[0].size = size;
        fb = datfs.tellg();
        datfs.seekg(0, ios_base::end);
        datfs.write((char*)&block, sizeof(EEmptyBlock));
        return;
    }

    datfs.seekg(fb, ios_base::beg);
    datfs.read((char*)&block, SEEBLOCK);
    while(block.curNum == PAGESIZE && block.nextBlock != -1)
    {
        datfs.seekg(block.nextBlock, ios_base::beg);
        datfs.read((char*)&block, SEEBLOCK);
    }

    if(block.curNum < PAGESIZE)
    {
        block.eles[block.curNum].pos = offset;
        block.eles[block.curNum].size = size;
        block.curNum++;
        datfs.seekg( -SEEBLOCK, ios_base::cur);
        datfs.write((char*)&block, SEEBLOCK);
    }
    else
    {
        EEmptyBlock block2;
        block2.eles[0].pos = offset;
        block2.eles[0].size = size;
        block2.curNum++;
        int curPos = datfs.tellg();

        datfs.seekg(0, ios_base::end);
        datfs.write((char*)&block, SEEBLOCK);

        block.nextBlock = datfs.tellg();
        block.nextBlock -= SEEBLOCK;

        datfs.seekg(curPos - SEEBLOCK, ios_base::beg);
        datfs.write((char*)&block, SEEBLOCK);
    }
}
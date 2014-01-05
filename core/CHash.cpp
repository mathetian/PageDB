#include "CHash.h"
#include <sys/stat.h>

#include "BufferPacket.h"

EmptyBlock::EmptyBlock() : curNum(0), nextBlock(-1)
{
}

EmptyBlock::~EmptyBlock()
{
}

bool EmptyBlock::checkSuitable(int size, int & pos)
{
    for(pos = curNum - 1; pos >= 0; pos--)
    {
        if(eles[pos].size > size)
            return true;
    }
    return false;
}

Chain::Chain(ChainHash * cHash, int defaultFirstOffset)
{
    this -> cHash = cHash;
    firstoffset = defaultFirstOffset;
}

Chain::~Chain()
{
}

bool Chain::put(const string&key,const string&value, int hashVal)
{
    if(check(key, hashVal) != 0)
        return false;

    Elem elem(firstoffset, key.size(), value.size());

    int offset = cHash -> findSuitable(key.size() + value.size() + SELEM);

    cHash -> datfs.seekg(offset, ios_base::beg);

    BufferPacket packet(SELEM + sizeof(key) + sizeof(value));
    packet << ((char*)&elem) << key << value;
    
    cHash -> datfs.write(packet.getData(),packet.getSize());
    
    firstoffset = offset;
    
    return true;
}

string   Chain::get(const string&key, int hashVal)
{
    int offset = firstoffset;
    Elem elem;
    while(offset != -1)
    {
        cHash -> datfs.seekg(offset, ios_base::beg);
        cHash -> datfs.read ((char*)&elem, SELEM);
        offset = elem.nextOffset;
        if(elem.hashVal != hashVal || elem.keySize != key.size())
            continue;
        
        BufferPacket packet(elem.keySize + elem.valueSize);
        cHash -> datfs.read(packet.getData(), packet.getSize());
        string key1,value1;
        packet >> key1 >> value1;
        if(key == key1) return value1;
    }
    return "";
}

bool   Chain::check(const string&key, int hashVal)
{
    int offset = firstoffset;
    Elem elem;
    while(offset != -1)
    {
        cHash -> datfs.seekg(offset, ios_base::beg);
        cHash -> datfs.read ((char*)&elem, SELEM);
        offset = elem.nextOffset;
        if(elem.hashVal != hashVal || elem.keySize != key.size())
            continue;
       
        BufferPacket packet(elem.keySize);
        cHash -> datfs.read(packet.getData(),packet.getSize());
        string key1(elem.keySize,0);
        packet >> key1;
        if(key == key1) return true;
    }
    return false;
}

bool Chain::remove(const string&key, int hashVal)
{
    int offset = firstoffset;
    Elem elem;
    int oldoffset = firstoffset;
    while(offset != -1)
    {
        cHash -> datfs.seekg(offset, ios_base::beg);
        cHash -> datfs.read ((char*)&elem, SELEM);
        oldoffset = offset;
        offset = elem.nextOffset;
        if(elem.hashVal != hashVal || elem.keySize != key.size())
            continue;
        BufferPacket packet(elem.keySize);
        cHash -> datfs.read(packet.getData(), packet.getSize());
        string key1(packet.getSize(), 0);
        packet >> key1;
        if(key == key1)
        {
            /**
            	Storage the emptry space into the the header linklist
            **/
            cHash -> cycle(oldoffset, SELEM + elem.keySize + elem.valueSize);
            return true;
        }
    }
    return false;
}

ChainHash::ChainHash(int chainCount, HASH hashFunc)
{
    this -> chainCount = chainCount;
    this -> hashFunc = hashFunc;
}

ChainHash::~ChainHash()
{
    datfs.close();
    int index = 0;
    for(; index < chainCount; index++)
    {
        delete headers[index];
        headers[index] = NULL;
    }
}

bool ChainHash::init(const string & filename)
{
    struct stat buf;
    string datFileName = filename + ".dat";

    datfs.open (datFileName.c_str(), std::fstream::in | std::fstream::out | std::fstream::app);

    if((stat(datFileName.c_str(), &buf) == -1) || buf.st_size == 0)
    {
        entries.push_back(0);
        entries = vector<int>(chainCount, -1);
        datfs.seekg(0,ios_base::beg);
        writeToFile();
    }
    else readFromFile();

    headers = vector <Chain*> (chainCount, NULL);
    int index = 0;
    for(; index < chainCount; index++)
        headers[index] = new Chain(this, entries.at(index));
}

void ChainHash::writeToFile()
{
    BufferPacket packet(SINT * 2 + SINT * chainCount);
    packet << chainCount << fb;
    packet.write((char*)&entries[0], SINT * chainCount);

    datfs.write(packet.getData(), packet.getSize());
}

void ChainHash::readFromFile()
{
    datfs.seekg(0, ios_base::beg);
    BufferPacket packet(SINT * 2 + SINT * chainCount);
    packet >> chainCount >> fb;
    
    entries = vector<int>(chainCount, -1);
    
    packet.read((char*)&entries[0],SINT * chainCount);
}

bool ChainHash::put(const string&key,const string&value)
{
    int hashVal = hashFunc(key);
    Chain * chain = headers.at(hashVal % chainCount);
    return chain -> put(key, value, hashVal);
}

string ChainHash::get(const string&key)
{
    int hashVal = hashFunc(key);
    Chain * chain = headers.at(hashVal % chainCount);
    return chain -> get(key, hashVal);
}

bool ChainHash::remove(const string&key)
{
    int hashVal = hashFunc(key);
    Chain * chain = headers.at(hashVal % chainCount);
    return chain -> remove(key, hashVal);
}

int ChainHash::findSuitable(int size)
{
    EmptyBlock block;
    int offset, pos;
    int blockDir;

    if(fb == -1)
    {
        block.curNum = 1;
        block.eles[0].pos = datfs.tellg();
        block.eles[0].size = size;
        offset = datfs.tellg();
        offset += size;
        datfs.seekg(2*size,ios_base::end);
        datfs.write((char*)&block, sizeof(EmptyBlock));
        return offset;
    }

    datfs.seekg(fb, ios_base::cur);
    datfs.write((char*)&block, sizeof(EmptyBlock));

    blockDir = fb;
    while(block.checkSuitable(size, pos) == false)
    {
        if(block.nextBlock == -1) break;
        blockDir = block.nextBlock;
        datfs.seekg(block.nextBlock, ios_base::beg);
        datfs.read((char*)&block, sizeof(EmptyBlock));
    }

    if(block.nextBlock == -1)
    {
        EmptyBlock nBlock;
        nBlock.curNum = 1;
        block.eles[0].pos = datfs.tellg();
        block.eles[0].size = size;
        offset = datfs.tellg();
        offset += size;
        datfs.seekg(2 * size, ios_base::end);
        datfs.write((char*)&block, SEBLOCK);
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
            datfs.write((char*)&block, SEBLOCK);
        }
        else
        {
            offset = block.eles[pos].pos;
            block.eles[pos].size -= size;
            datfs.seekg(blockDir, ios_base::beg);
            datfs.write((char*)&block, SEBLOCK);
        }
    }
    return offset;
}

void ChainHash::cycle(int offset, int size)
{
    EmptyBlock block;

    if(fb == -1)
    {
        block.curNum = 1;
        block.eles[0].pos = offset;
        block.eles[0].size = size;
        fb = datfs.tellg();
        datfs.seekg(0, ios_base::end);
        datfs.write((char*)&block, sizeof(EmptyBlock));
        return;
    }

    datfs.seekg(fb, ios_base::beg);
    datfs.read((char*)&block, SEBLOCK);
    while(block.curNum == PAGESIZE && block.nextBlock != -1)
    {
        datfs.seekg(block.nextBlock, ios_base::beg);
        datfs.read((char*)&block, SEBLOCK);
    }

    if(block.curNum < PAGESIZE)
    {
        block.eles[block.curNum].pos = offset;
        block.eles[block.curNum].size = size;
        block.curNum++;
        datfs.seekg( -SEBLOCK, ios_base::cur);
        datfs.write((char*)&block, SEBLOCK);
    }
    else
    {
        EmptyBlock block2;
        block2.eles[0].pos = offset;
        block2.eles[0].size = size;
        block2.curNum++;
        int curPos = datfs.tellg();

        datfs.seekg(0, ios_base::end);
        datfs.write((char*)&block, SEBLOCK);

        block.nextBlock = datfs.tellg();
        block.nextBlock -= SEBLOCK;

        datfs.seekg(curPos - SEBLOCK, ios_base::beg);
        datfs.write((char*)&block, SEBLOCK);
    }
}
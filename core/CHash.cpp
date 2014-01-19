#include "FactoryImpl.h"

bool CEmptyBlock::checkSuitable(int size, int & pos)
{
    for(pos = curNum - 1; pos >= 0; pos--)
    {
        if(eles[pos].size > size)
            return true;
    }
    return false;
}

CEmptyBlock CEmptyBlock::split()
{
    CEmptyBlock newblock;
    
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

bool Chain::put(const Slice & key,const Slice & value, int hashVal)
{
    if(check(key, hashVal) != 0)
        return false;

    CElement elem(firstoffset, key.size(), value.size(), hashVal);

    int offset = cHash -> findSuitableOffset(key.size() + value.size() + SELEM);

    cHash -> datfs.seekg(offset, ios_base::beg);

    BufferPacket packet(SELEM + key.size() + value.size());
    Slice slice((char*)&elem, SELEM); /**Need to record hashVal?**/
    packet << slice << key << value;
    
    cHash -> datfs.write(packet.getData(),packet.getSize());
    
    firstoffset = offset;
    
    return true;
}

Slice   Chain::get(const Slice & key, int hashVal)
{
    int offset = firstoffset;
    CElement elem;

    while(offset != -1)
    {
        cHash -> datfs.seekg(offset, ios_base::beg);
        cHash -> datfs.read ((char*)&elem, SELEM);
        
        offset = elem.nextOffset;
        
        if(elem.hashVal != hashVal || elem.keySize != key.size())
            continue;
        
        BufferPacket packet(elem.keySize + elem.valueSize);
        Slice key1(elem.keySize), value1(elem.valueSize);

        cHash -> datfs.read(packet.getData(), packet.getSize());
        
        packet >> key1 >> value1;

        if(key == key1) return value1;
    }
    return "";
}

bool   Chain::check(const Slice & key, int hashVal)
{
    int offset = firstoffset;
    CElement elem;

    while(offset != -1)
    {
        cHash -> datfs.seekg(offset, ios_base::beg);
        cHash -> datfs.read ((char*)&elem, SELEM);
        offset = elem.nextOffset;
        if(elem.hashVal != hashVal || elem.keySize != key.size())
            continue;
       
        BufferPacket packet(elem.keySize);
        cHash -> datfs.read(packet.getData(),packet.getSize());

        Slice slice(elem.keySize);
        if(key == slice) return true;
    }

    return false;
}

bool Chain::remove(const Slice & key, int hashVal)
{
    int offset = firstoffset, oldoffset = offset;
    CElement elem;

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
        
        Slice slice(elem.keySize);
        packet >> slice;

        if(key == slice)
        {
            /**
            	Storage the emptry space into the the header linklist
            **/
            cHash -> recycle(oldoffset, SELEM + elem.keySize + elem.valueSize);
            return true;
        }
    }
    return false;
}

ChainHash::~ChainHash()
{
    datfs.close();
    
    for(int index = 0;index < chainCount; index++)
    {
        delete headers[index];
        headers[index] = NULL;
    }
}

bool ChainHash::init(const char * filename)
{
    struct stat buf;
    string datFileName = filename;
    datFileName += ".dat";

    /**Need process it like EHash**/
    datfs.open (datFileName.c_str(), std::fstream::in | std::fstream::out | std::fstream::app);

    if((stat(datFileName.c_str(), &buf) == -1) || buf.st_size == 0)
    {
        entries = vector<int>(chainCount, -1);

        writeToFile();
    }
    else readFromFile();

    headers = vector <Chain*> (chainCount, NULL);

    for(int index = 0;index < chainCount;index++)
        headers[index] = new Chain(this, entries.at(index));
}

void ChainHash::writeToFile()
{
    BufferPacket packet(SINT * 2 + SINT * chainCount);
    Slice slice((char*)&entries[0], SINT * chainCount);

    packet << chainCount << fb << slice;

    datfs.seekg(0,ios_base::beg);
    datfs.write(packet.getData(), packet.getSize());
}

void ChainHash::readFromFile()
{
    BufferPacket packet(SINT * 2 + SINT * chainCount);
    packet >> chainCount >> fb;
    
    entries = vector<int>(chainCount, -1);
    
    datfs.seekg(0, ios_base::beg);
    packet.read((char*)&entries[0],SINT * chainCount);
}

bool ChainHash::put(const Slice & key,const Slice & value)
{
    int hashVal   = hashFunc(key);
    Chain * chain = headers.at(hashVal % chainCount);
    /**Problem ?**/
    return chain -> put(key, value, hashVal);
}

Slice ChainHash::get(const Slice & key)
{
    int hashVal = hashFunc(key);
    Chain * chain = headers.at(hashVal % chainCount);
    /**Problem ?**/
    return chain -> get(key, hashVal);
}

bool ChainHash::remove(const Slice & key)
{
    int hashVal = hashFunc(key);
    Chain * chain = headers.at(hashVal % chainCount);
     /**Problem ?**/
    return chain -> remove(key, hashVal);
}

int ChainHash::findSuitableOffset(int size)
{
    CEmptyBlock block;

    int offset, pos, blockDir;

    if(fb == -1)
    {
        datfs.seekg(0, ios_base::end);
        
        this -> fb = datfs.tellg();

        block.eles[0].pos  = fb + SCEBLOCK;
        block.eles[0].size = size;
        block.curNum       = 1;

        datfs.write((char*)&block, SCEBLOCK);

        datfs.seekg(SINT, ios_base::beg);
        datfs.write((char*)&fb, SINT);

        return fb + SCEBLOCK + size;
    }

    datfs.seekg(fb, ios_base::beg);
    datfs.read((char*)&block, SCEBLOCK);
    
    if(block.nextBlock != -1)
    {
        if(block.curNum < PAGESIZE/2)
        {
            CEmptyBlock nnBlock;
            
            int old = block.nextBlock, index = 0, bnum = block.curNum;

            datfs.seekg(block.nextBlock, ios_base::beg);
            datfs.read((char*)&nnBlock, SCEBLOCK);
                        
            block.nextBlock = nnBlock.nextBlock;

            datfs.seekg(0, ios_base::end);

            for(;index < nnBlock.curNum;index++)
            {
                block.eles[block.curNum++] = nnBlock.eles[index];
                if(block.curNum == PAGESIZE) 
                {
                    CEmptyBlock nnn = block.split();
                    nnn.nextBlock   = block.nextBlock;
                    block.nextBlock = datfs.tellg();
                    
                    datfs.write((char*)&nnn, SCEBLOCK);
                }
            }
            if(block.curNum == PAGESIZE)
            {
                CEmptyBlock nnn = block.split();
                nnn.nextBlock   = block.nextBlock;
                block.nextBlock = datfs.tellg();
                
                datfs.write((char*)&nnn, SCEBLOCK);
            }
            block.eles[block.curNum].pos    = old;
            block.eles[block.curNum++].size = SCEBLOCK;
        }
    }

    if(block.checkSuitable(size, pos) == true)
    {
        CEmptyBlock::CEmptyEle ele = block.eles[pos];

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
            
            CEmptyBlock nnn = block.split();
            nnn.nextBlock   = block.nextBlock;
            block.nextBlock = datfs.tellg();
            
            datfs.write((char*)&nnn, SCEBLOCK);
        }

        datfs.seekg(2 * size,ios_base::end);
        
        offset = datfs.tellg(); offset -= size;

        block.eles[block.curNum].pos    = offset - size;
        block.eles[block.curNum++].size = size;
    }   

    datfs.seekg(fb, ios_base::beg);
    datfs.write((char*)&block, SCEBLOCK);

    return offset;
}

void ChainHash::recycle(int offset, int size)
{
    CEmptyBlock block;

    if(fb == -1)
    {
        log -> _Fatal("There must be some fatal error\n");
        return;
    }

    datfs.seekg(fb,      ios_base::beg);
    datfs.read((char*)&block, SCEBLOCK);
    
    if(block.curNum == PAGESIZE)
    {
        datfs.seekg(0, ios_base::end);

        int nn = block.nextBlock;
        
        CEmptyBlock nnBlock = block.split();
        nnBlock.nextBlock   = nn;
        block.nextBlock     = datfs.tellg();

        datfs.write((char*)&nnBlock, SCEBLOCK);
    }

    block.eles[block.curNum].pos    = offset;
    block.eles[block.curNum++].size = size;

    datfs.seekg(fb, ios_base::beg);
    datfs.write((char*)&block,SCEBLOCK);
}
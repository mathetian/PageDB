#include "ChainHashDBImpl.h"

namespace customdb
{

ChainDB::ChainDB(int chainCount, HASH hashFunc) :\
    chainCount(chainCount), hashFunc(hashFunc) { }

ChainDB::~ChainDB()
{
    datfs.close();

    for(int index = 0; index < chainCount; index++)
    {
        delete headers[index];
        headers[index] = NULL;
    }
}

bool     ChainDB::put(const Slice & key,const Slice & value)
{
    uint32_t hashVal   = hashFunc(key);
    ChainTable * chain = headers.at(hashVal % chainCount);
    return chain -> put(key, value, hashVal);
}

Slice    ChainDB::get(const Slice & key)
{
    uint32_t hashVal = hashFunc(key);
    ChainTable * chain = headers.at(hashVal % chainCount);
    /**Problem ?**/
    return chain -> get(key, hashVal);
}

bool     ChainDB::remove(const Slice & key)
{
    uint32_t hashVal = hashFunc(key);
    ChainTable * chain = headers.at(hashVal % chainCount);
    /**Problem ?**/
    return chain -> remove(key, hashVal);
}

bool	 ChainDB::init(const char * filename)
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

    headers = vector <ChainTable*> (chainCount, NULL);

    for(int index = 0; index < chainCount; index++)
        headers[index] = new ChainTable(this, entries.at(index));
}

void     ChainDB::removeAll(const char * filename)
{

}

void     ChainDB::dump()
{
    //Todo list
}

void     ChainDB::fflush()
{
    //Todo list
}

void     ChainDB::runBatch(const WriteBatch * pbatch)
{
    //Todo list
}

void     ChainDB::runBatchParallel(const WriteBatch * pbatch)
{
    //Todo list
}

void     ChainDB::write(WriteBatch* pbatch)
{
    //Todo list
}

void     ChainDB::compact()
{
    //Todo list
}


void     ChainDB::recycle(int offset, int size)
{

}

void     ChainDB::writeToFile()
{
    BufferPacket packet(SINT * 2 + SINT * chainCount);
    Slice slice((char*)&entries[0], SINT * chainCount);

    packet << chainCount << fb << slice;

    datfs.seekg(0,ios_base::beg);
    datfs.write(packet.getData(), packet.getSize());
}

void     ChainDB::readFromFile()
{
    BufferPacket packet(SINT * 2 + SINT * chainCount);
    packet >> chainCount >> fb;

    entries = vector<int>(chainCount, -1);

    datfs.seekg(0, ios_base::beg);
    packet.read((char*)&entries[0],SINT * chainCount);
}

int      ChainDB::findSuitableOffset(int size)
{
    ChainEmptyBlock block;

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
            ChainEmptyBlock nnBlock;

            int old = block.nextBlock, index = 0, bnum = block.curNum;

            datfs.seekg(block.nextBlock, ios_base::beg);
            datfs.read((char*)&nnBlock, SCEBLOCK);

            block.nextBlock = nnBlock.nextBlock;

            datfs.seekg(0, ios_base::end);

            for(; index < nnBlock.curNum; index++)
            {
                block.eles[block.curNum++] = nnBlock.eles[index];
                if(block.curNum == PAGESIZE)
                {
                    ChainEmptyBlock nnn = block.split();
                    nnn.nextBlock   = block.nextBlock;
                    block.nextBlock = datfs.tellg();

                    datfs.write((char*)&nnn, SCEBLOCK);
                }
            }
            if(block.curNum == PAGESIZE)
            {
                ChainEmptyBlock nnn = block.split();
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
        ChainEmptyBlock::CEmptyEle ele = block.eles[pos];

        offset = ele.pos;
        ele.pos += size;

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

            ChainEmptyBlock nnn = block.split();
            nnn.nextBlock   = block.nextBlock;
            block.nextBlock = datfs.tellg();

            datfs.write((char*)&nnn, SCEBLOCK);
        }

        datfs.seekg(2 * size,ios_base::end);

        offset = datfs.tellg();
        offset -= size;

        block.eles[block.curNum].pos    = offset - size;
        block.eles[block.curNum++].size = size;
    }

    datfs.seekg(fb, ios_base::beg);
    datfs.write((char*)&block, SCEBLOCK);

    return offset;
}

void ChainDB::removeDB(const char * filename)
{

}

};
#include "ChainHashDBImpl.h"

namespace customdb
{

ChainEmptyBlock::ChainEmptyBlock() : curNum(0), nextBlock(-1)
{

}

bool ChainEmptyBlock::checkSuitable(int size, int & pos)
{
 	for(pos = curNum - 1; pos >= 0; pos--)
    {
        if(eles[pos].size > size)
            return true;
    }
    return false;
}

void ChainEmptyBlock::newBlock(int size)
{
	//Todo list
}

ChainEmptyBlock ChainEmptyBlock::split()
{
	ChainEmptyBlock newblock;

    int cn1 = 0, cn2 = 0;

    for(int index = 0; index < curNum; index++)
    {
        if(index & 1) newblock.eles[cn1++] = eles[index];
        else eles[cn2++] = eles[index];
    }

    newblock.curNum = cn1;
    curNum          = cn2;

    return newblock;
}

};
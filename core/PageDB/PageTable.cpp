#include "PageDBImpl.h"

namespace customdb
{

PageTable::PageTable(PageDB * db) : d(0), curNum(0), db(db)
{
    m_log = Log::GetInstance();
}

BufferPacket PageTable::getPacket()
{
    BufferPacket packet(SPAGETABLE);
    Slice slice((char*)&elements[0], sizeof(elements));

    packet << d << curNum << slice;

    return packet;
}

void PageTable::setByBucket(BufferPacket & packet)
{
    packet.reset();

    packet >> d >> curNum;

    packet.read((char*)&elements[0],sizeof(elements));
}

bool   PageTable::full()
{
    if(curNum >= PAGESIZE + 3)
        printf("xxx:%d\n", curNum);
    assert(curNum < PAGESIZE + 3);

    return curNum >= PAGESIZE;
}

bool   PageTable::put(const Slice & key,const Slice & value, uint32_t hashVal)
{
    for(int index = 0; index < curNum; index++)
    {
        PageElement element = elements[index];
        if(element.m_hashVal == hashVal && element.m_keySize == \
                key.size() && element.m_datSize == value.size())
        {
            BufferPacket packet(element.m_keySize);
            Slice        slice(element.m_keySize);

            db -> m_datfile.Read(packet.str(), element.m_datPos, packet.size());
            packet.reset();
            packet >> slice;
            if(slice == key)
                return 0;
        }
    }

    /**
        Find an suitable empty block,
        if not allocated it at the end of the file
    **/

    BufferPacket packet(key.size() + value.size());
    packet << key << value;
    int offset = db -> findSuitableOffset(packet.size());
    db -> m_datfile.Write(packet.c_str(), offset, packet.size());
    /**
        Modify the page index
    **/

    elements[curNum].m_hashVal   = hashVal;
    elements[curNum].m_datPos    = offset;
    elements[curNum].m_keySize   = key.size();
    elements[curNum++].m_datSize = value.size();

    return 1;
}

Slice  PageTable::get(const Slice & key, uint32_t hashVal)
{
    for(int index = 0; index < curNum; index++)
    {
        if(elements[index].m_hashVal == hashVal && elements[index].m_keySize == key.size())
        {
            BufferPacket packet(elements[index].m_datSize + elements[index].m_keySize);

            Slice slice1(elements[index].m_datSize);
            Slice slice2(elements[index].m_keySize);

            db -> m_datfile.Read(packet.str(), elements[index].m_datPos, packet.size());
            packet >> slice2 >> slice1;

            if(slice2 == key)
                return slice1;
        }
    }
    return "";
}

bool   PageTable::remove(const Slice & key, uint32_t hashVal)
{
    int index;
    for(index = 0; index < curNum; index++)
    {
        if(elements[index].m_hashVal == hashVal && elements[index].m_keySize == key.size())
        {
            BufferPacket packet(elements[index].m_keySize);
            Slice slice(elements[index].m_keySize);

            db -> m_datfile.Read(packet.str(), elements[index].m_datPos, packet.size());
            packet >> slice;

            if(slice == key) break;
        }
    }

    if(index == curNum) return false;

    /**
      Attach the space to the emptryBlock
    **/
    db -> recycle(elements[index].m_datPos, elements[index].m_keySize + elements[index].m_datSize);

    for(; index < curNum - 1; index++)
        elements[index] = elements[index + 1];

    curNum --;

    return true;
}

void   PageTable::replaceQ(const Slice & key, const Slice & value, uint32_t hashVal, int offset)
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
                ScopeMutex scope(&(db -> datLock));
                db -> m_datfile.Read(packet.str(), element.m_datPos, packet.size());
            }

            packet >> slice;

            if(slice != key)
            {
                continue;
            }

            db -> recycle(elements[index].m_datPos, key.size() + value.size());

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

void PageTable::addElement(const PageElement & element)
{
    elements[curNum++] = element;
}

PageElement PageTable::getElement(int index)
{
    return elements[index];
}

PageElement PageTable::getElement2(int index)
{
    return elements[index];
}

int    PageTable::getCurNum() const
{
    return curNum;
}

int    PageTable::getD() const
{
    return d;
}

void   PageTable::setCurNum(int num)
{
    curNum = num;
}
void   PageTable::setD(int d)
{
    this -> d = d;
}

};
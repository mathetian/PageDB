// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "PageDBImpl.h"

namespace customdb
{

PageTable::PageTable(PageDB * db) : m_d(0), m_curNum(0), m_db(db)
{

}

/**
** Buffer <-> PageContent
**/
BufferPacket PageTable::getPacket()
{
    BufferPacket packet(SPAGETABLE);
    Slice slice((char*)&m_elements[0], sizeof(m_elements));

    packet << m_d << m_curNum << slice;

    return packet;
}

void PageTable::setByBucket(BufferPacket & packet)
{
    packet.reset();
    packet >> m_d >> m_curNum;
    packet.read((char*)&m_elements[0],sizeof(m_elements));
}

/**
** We add some redundancy in PageElements
** When the size exceed PageSize+3, fatal error
**/
bool   PageTable::full()
{
    if(m_curNum >= PAGESIZE + 3)
        printf("xxx:%d\n", m_curNum);

    assert(m_curNum < PAGESIZE + 3);

    return m_curNum >= PAGESIZE;
}

/**
** put method
** Return false, if duplicate
** Return true, if not exist
**/
bool   PageTable::put(const Slice & key, const Slice & value, uint32_t hashVal, uint64_t offset)
{
    int index = -1;
    bool flag;
    flag = find(key, hashVal, index);

    if(flag == true)
        m_db -> recycle(m_elements[index].m_datPos, m_elements[index].m_keySize + m_elements[index].m_datSize);

    if(flag == true)
    {
        m_elements[index].m_hashVal   = hashVal;
        m_elements[index].m_datPos    = offset;
        m_elements[index].m_keySize   = key.size();
        m_elements[index].m_datSize = value.size();
    }
    else
    {
        m_elements[m_curNum].m_hashVal   = hashVal;
        m_elements[m_curNum].m_datPos    = offset;
        m_elements[m_curNum].m_keySize   = key.size();
        m_elements[m_curNum++].m_datSize = value.size();
    }

    return !flag;
}

/**
** get method
** Return "", if not exist
** Return the actual value
**/
Slice  PageTable::get(const Slice & key, uint32_t hashVal)
{
    for(int index = 0; index < m_curNum; index++)
    {
        if(m_elements[index].m_hashVal == hashVal && m_elements[index].m_keySize == key.size())
        {
            BufferPacket packet(m_elements[index].m_datSize + m_elements[index].m_keySize);

            Slice internal_key(m_elements[index].m_datSize);
            Slice internal_value(m_elements[index].m_keySize);

            m_db -> m_datfile.Read(packet.str(), m_elements[index].m_datPos, packet.size());
            packet >> internal_key >> internal_value;

            if(internal_key == key)
                return internal_value;
        }
    }
    return "";
}

/**
** remove method
** Return false, if not exist
** Return true, if exist
**/
bool   PageTable::remove(const Slice & key, uint32_t hashVal)
{
    int index;
    PageElement element;
    for(index = 0; index < m_curNum; index++)
    {
        element = m_elements[index];
        if(element.m_hashVal == hashVal && element.m_keySize == key.size())
        {
            BufferPacket packet(element.m_keySize);
            Slice internal_key(element.m_keySize);

            m_db -> m_datfile.Read(packet.str(), element.m_datPos, packet.size());
            packet >> internal_key;

            if(internal_key == key) break;
        }
    }

    if(index == m_curNum) return false;

    /**
      Attach the space to the emptryBlock
    **/
    m_db -> recycle(element.m_datPos, element.m_keySize + element.m_datSize);

    for(; index < m_curNum - 1; index++)
        m_elements[index] = m_elements[index + 1];

    m_curNum --;

    return true;
}

/**
** Judge whether key is duplicate in the elements
** Return true, if exist the same key. Set the approxiate index
** Return false, if not exist the same key
**/
bool   PageTable::find(const Slice & key, uint32_t hashVal, int &index)
{
    for(index = 0; index < m_curNum; index++)
    {
        PageElement element = m_elements[index];
        if(element.m_hashVal == hashVal && element.m_keySize == key.size())
        {

            BufferPacket packet(element.m_keySize);
            Slice        internal_key(element.m_keySize);

            m_db -> m_datfile.Read(packet.str(), element.m_datPos, packet.size());
            packet >> internal_key;

            if(internal_key != key) continue;
            return true;
        }
    }

    return false;
}

};
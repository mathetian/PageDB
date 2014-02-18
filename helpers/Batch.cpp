#include "Batch.h"

namespace customdb
{

WriteBatch::WriteBatch(int size) : \
    m_size(size), m_num(0), m_msize(0) \
{ m_ssvec = vector<Node>(m_size);}

void WriteBatch::put(const Slice& key, const Slice& value)
{
    if(m_num == m_size)
    {
        m_size *= 2;
        m_ssvec.resize(m_size);
    }

    m_ssvec[m_num].first = key;
    m_ssvec[m_num++].second = value;

    m_msize += key.size() + value.size();
}

void WriteBatch::remove(const Slice& key)
{
    //Todo list
}

void WriteBatch::clear()
{
    vector<Node> empty = vector<Node>(m_size);
    swap(m_ssvec, empty);

    assert(m_ssvec.size() == m_size);
    m_num = 0;
    m_msize = 0;
}

uint32_t WriteBatch::getTotalSize() const
{
    return m_msize;
}

int      WriteBatch::getCount() const
{
    return m_num;
}


WriteBatch::Iterator::Iterator(const WriteBatch * pbatch) : \
    m_pbatch(pbatch), m_curNum(0) { }

const WriteBatch::Node * WriteBatch::Iterator::next()
{
    if(m_pbatch->m_num == m_curNum) return NULL;
    return &(m_pbatch -> m_ssvec[m_curNum++]);
}

const WriteBatch::Node * WriteBatch::Iterator::prev()
{
    if(m_curNum == 0) return NULL;
    return &(m_pbatch->m_ssvec[m_curNum--]);
}

void WriteBatch::Iterator::seekToFirst()
{
    m_curNum = 0;
}

void WriteBatch::Iterator::seekToEnd()
{
    m_curNum = m_pbatch->m_num;
}

const WriteBatch::Node * WriteBatch::Iterator::first()
{
    seekToFirst();
    return next();
}

const WriteBatch::Node * WriteBatch::Iterator::end()
{
    return NULL;
}

void WriteBatchInternal::Append(WriteBatch * dst, const WriteBatch * src)
{
    WriteBatch::Iterator iterator(src);

    for(const Node * node = iterator.first(); node != iterator.end(); \
            node = iterator.next()) dst->put(node->first, node->second);
}

uint32_t WriteBatchInternal::ByteSize(WriteBatch * dst)
{
    return dst->getTotalSize();
}

int WriteBatchInternal::Count(WriteBatch * dst)
{
    return dst->getCount();
}

};
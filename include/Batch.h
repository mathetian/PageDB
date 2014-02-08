#ifndef _BATCH_H
#define _BATCH_H

#include <vector>
using namespace std;
#include <stdint.h>

#include "Slice.h"

class WriteBatch {
public:
  typedef pair<Slice, Slice> Node;

public:
  WriteBatch(int size = 1000) : m_size(size), m_num(0), m_msize(0) \
      { m_ssvec = vector<Node>(m_size, make_pair(0,0));}
  
  ~WriteBatch() { }

  void put(const Slice& key, const Slice& value)
  {
    if(m_num == m_size) 
    {
      m_size *= 2;
      m_ssvec.resize(m_size, make_pair(0,0));
    }
      
    
    m_ssvec[m_num].first = key;
    m_ssvec[m_num++].second = value;

    m_msize += key.size() + value.size();
  }

  void remove(const Slice& key) 
  {
    /**
        Need other internel structure other than vector pair
    **/
  }

  void clear()
  {
    vector<Node> empty;
    swap(m_ssvec, empty);

    assert(m_ssvec.size() == 0);
    m_num = 0;
  }

  uint32_t getTotalSize() const { return m_msize; }

  int      getCount() const { return m_ssvec.size(); }

public:
  class Iterator {
  public:
    Iterator(const WriteBatch * pbatch) : m_pbatch(pbatch), m_curNum(0) { } 

  public:
    const Node * next()
    {
      if(m_pbatch->m_num == m_curNum) return NULL;
      return &(m_pbatch -> m_ssvec[m_curNum++]);
    }

    const Node * prev()
    {
      if(m_curNum == 0) return NULL;
      return &(m_pbatch->m_ssvec[m_curNum--]);
    }

    void seekToFirst() { m_curNum = 0; }

    void seekToEnd() { m_curNum = m_pbatch->m_num; }

    const Node * first()
    {
      seekToFirst();
      return next();
    }

    const Node * end()
    {
      return NULL;
    }
    
  private:
    const WriteBatch* m_pbatch;
    int m_curNum;
  };

private:
  vector<Node> m_ssvec;

  int m_size;
  int m_num;
  uint32_t m_msize;
};

class ReadBatch{
public:
  ReadBatch();
};

class WriteBatchInternal{
public:
  typedef pair<Slice, Slice> Node;

public:
  static void Append(WriteBatch * dst, const WriteBatch * src)
  {
      WriteBatch::Iterator iterator(src);
      for(const Node * node = iterator.first();node != iterator.end();\
        node = iterator.next()) dst->put(node->first, node->second);
  }

  static uint32_t ByteSize(WriteBatch * dst)
  {
    return dst->getTotalSize();
  }

  static int Count(WriteBatch * dst)
  {
    return dst->getCount();
  }
};
#endif
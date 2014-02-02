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
    if(m_num < m_size)
    {
      m_ssvec[m_num++] = make_pair(key, value);
    }
    else
    {
      m_ssvec.push_back(make_pair(key,value));
      m_num++;
    }
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
    while(m_num--) m_ssvec.pop_back();
    m_num = 0;
  }

  uint32_t getTotalSize() const { return m_msize; }

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
#endif
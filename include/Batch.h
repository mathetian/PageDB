#ifndef _BATCH_H
#define _BATCH_H

#include <vector>
using namespace std;

#include <assert.h>
#include <stdint.h>

#include "Slice.h"


namespace customdb{

class WriteBatch {
private:
  typedef pair<Slice, Slice> Node;

public:
  WriteBatch(int size = 1000);

public:
  void     put(const Slice& key, const Slice& value);
  void     remove(const Slice& key) ;
  void     clear();
  uint32_t getTotalSize() const;
  int      getCount() const;

public:
  class Iterator {
  public:
    Iterator(const WriteBatch * pbatch);
    const Node * next();
    const Node * prev();
    void seekToFirst();
    void seekToEnd();
    const Node * first();
    const Node * end();
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

//Todo list
class ReadBatch{
public:
};

class WriteBatchInternal{
public:
  typedef pair<Slice, Slice> Node;

public:
  static void Append(WriteBatch * dst, const WriteBatch * src);
  static uint32_t ByteSize(WriteBatch * dst);
  static int Count(WriteBatch * dst);
};

};
#endif
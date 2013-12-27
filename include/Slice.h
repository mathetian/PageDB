#ifndef _SLICE_H
#define _SLICE_H

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <string>
#include <iostream>
using namespace std;

class Slice{
public:
  Slice() : 
  		_data(""), _size(0) {}
  
  Slice(const char* d, size_t n) : 
  		_data(d), _size(n) {}
  
  Slice(const std::string& s) : 
  		_data(s.data()), _size(s.size()) {}
  
  Slice(const char* s) : 
  		_data(s), _size(strlen(s)) {}

public:
  const char* tochars()  const { return _data; }
  
  std::string toString() const { return std::string(_data, _size); }

  size_t size() const { return _size; }
  
  bool  empty() const { return _size == 0; }

  void clear() { _data = ""; _size = 0; }

public:
  char operator[](size_t n) const 
  {
    assert(n < size());
    return _data[n];
  }

  Slice& operator+=(const Slice&s1);

private:
	const char * _data;
	size_t       _size;
};

bool operator==(const Slice&s1, const Slice&s2);
bool operator< (const Slice&s1, const Slice&s2);
bool operator> (const Slice&s1, const Slice&s2);

inline ostream & operator << (ostream & os, Slice &s1)
{
	os << s1.toString();
	return os;
}
#endif
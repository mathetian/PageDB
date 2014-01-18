#ifndef _BUF_PAC_H
#define _BUF_PAC_H

#include <string>
using namespace std;

#include <string.h>
#include "../include/Log.h"
#include "../include/Slice.h"

typedef unsigned long long uint64_t;

class BufferPacket{
public:
	BufferPacket(int size);
   ~BufferPacket();

public:
	BufferPacket & operator << (int ivalue);
	BufferPacket & operator << (size_t st);
	BufferPacket & operator << (const string & str);
	BufferPacket & operator << (const Slice & slice);
	BufferPacket & operator << (const char * str);
	BufferPacket & operator << (const BufferPacket & packet);

public:
	BufferPacket & operator >> (int    & ivalue);
	BufferPacket & operator >> (size_t & st);
	BufferPacket & operator >> (string & str);
	BufferPacket & operator >> (Slice  & slice);
	BufferPacket & operator >> (char * str);

public:
	/**Sorry for that**/
	BufferPacket & write(const char * str, int len);
	BufferPacket & read (char * str, int len);

public:
	char * getData() { return data;}
	int    getSize() { return size;}
	void   setBeg()  { cur = 0;}

private:
	char * data;
	int    size, cur;
	Log  * log;
};

#endif
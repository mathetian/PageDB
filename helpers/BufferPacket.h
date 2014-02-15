#ifndef _BUF_PAC_H
#define _BUF_PAC_H

#include <string>
using namespace std;

#include <string.h>
#include "Log.h"
#include "Slice.h"
#include <stdint.h>

//typedef unsigned long long uint64_t;

class BufferPacket{
public:
	BufferPacket(int size);
   ~BufferPacket();
    /**How big the bug is**/
    BufferPacket(const BufferPacket & packet)
    {
    	data = new char[packet.size];
    	memcpy(data, packet.data, packet.size);

    	size = packet.size;
    	cur  = packet.cur;
    }

    BufferPacket & operator=(const BufferPacket & packet)
    {
    	if(size != 0) delete [] data;
    	
    	data = new char[packet.size];
    	memcpy(data, packet.data, packet.size);

    	size = packet.size;
    	cur  = packet.cur;

    	return *this;
    }

public:
	BufferPacket & operator << (int ivalue);
	BufferPacket & operator << (size_t st);
	BufferPacket & operator << (const string & str);
	BufferPacket & operator << (const Slice & slice);
	BufferPacket & operator << (const char * str);
	BufferPacket & operator << (const BufferPacket & packet);
	BufferPacket & operator << (const uint32_t value);
	// BufferPacket & operator << (volatile int value);

public:
	BufferPacket & operator >> (int    & ivalue);
	BufferPacket & operator >> (size_t & st);
	BufferPacket & operator >> (string & str);
	BufferPacket & operator >> (Slice  & slice);
	BufferPacket & operator >> (char * str);
	BufferPacket & operator >> (uint32_t & value);
	// BufferPacket & operator >> (volatile int & value);

public:
	/**Sorry for that**/
	void write(const char * str, int len);
	void read (char * str, int len);

public:
	char * getData() { return data;}
	int    getSize() const { return size;}
	void   setBeg()  { cur = 0;}

private:
	char * data;
	int    size, cur;
	Log  * log;
};

#endif
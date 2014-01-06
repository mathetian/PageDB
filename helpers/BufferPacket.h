#ifndef _BUF_PAC_H
#define _BUF_PAC_H

#include <string>
using namespace std;

#include <string.h>
#include "Log.h"

typedef unsigned long long uint64_t;

class BufferPacket{
public:
	BufferPacket(int size);
   ~BufferPacket();

public:
	BufferPacket & operator << (int ivalue);
	BufferPacket & operator << (size_t st);
	BufferPacket & operator << (string str);
	void		   write       (char * str, int len);
	void		   write       (BufferPacket packet);

public:
	BufferPacket & operator >> (int&ivalue);
	BufferPacket & operator >> (size_t&st);
	BufferPacket & operator >> (string&str);
	void 		   read        (char * str, int len);
	
public:
	char * getData() { return data;}
	int    getSize() { return size;}
	void   setBeg()  { cur = 0;}

private:
	char * data;
	int    size, cur;
	Log  * log;
};

/*inline istream & operator << (istream & is, const BufferPacket & packet)
{
	cout<<"what?"<<endl;
	return is;
}*/

#endif
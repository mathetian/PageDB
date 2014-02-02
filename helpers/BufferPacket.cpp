#include "BufferPacket.h"

BufferPacket::BufferPacket(int size)
{
	this -> size = size;
	this -> data = new char[size];
	memset(this -> data, 0xff, size);
	this -> cur  = 0;
	log = Log::GetInstance();
}

BufferPacket::~BufferPacket()
{
	if(data) delete [] data;
	data = NULL;
}

BufferPacket & BufferPacket::operator << (int ivalue)
{
	if(cur + sizeof(int) > size)
		log -> _Error("BufferPacket error << int\n");
	else
	{
		*(int*)(data + cur) = ivalue;
		cur += sizeof(int);
	}
	return *this;
}

BufferPacket & BufferPacket::operator << (const uint32_t ivalue)
{
	if(cur + sizeof(uint32_t) > size)
		log -> _Error("BufferPacket error << int\n");
	else
	{
		*(uint32_t*)(data + cur) = ivalue;
		cur += sizeof(uint32_t);
	}
	return *this;
}

BufferPacket & BufferPacket::operator << (size_t st)
{
	if(cur + sizeof(size_t) > size)
		log -> _Error("BufferPacket error << size_t\n");
	else
	{
		*(size_t*)(data + cur) = st;
		cur += sizeof(size_t);
	}
	return *this;
}

BufferPacket & BufferPacket::operator << (const string & str)
{
	if(cur + str.size() > size)
		log -> _Error("BufferPacket error << string\n");
	else
	{
		memcpy(data + cur, str.c_str(), str.size());
		cur += str.size();
	}
	return *this;
}


BufferPacket & BufferPacket::operator >> (int&ivalue)
{
	if(cur + sizeof(ivalue) > size) 
		log -> _Error("BufferPacket >> int overflow\n");
	else
	{
		ivalue = *(int*)(data + cur);
		cur += sizeof(ivalue);
	}
	return *this;
}

BufferPacket & BufferPacket::operator >> (uint32_t & ivalue)
{
	if(cur + sizeof(ivalue) > size) 
		log -> _Error("BufferPacket >> int overflow\n");
	else
	{
		ivalue = *(uint32_t*)(data + cur);
		cur += sizeof(uint32_t);
	}
	return *this;
}

BufferPacket & BufferPacket::operator >> (size_t&st)
{
	if(cur + sizeof(st) > size)
		log -> _Error("BufferPacket >> size_t overflow\n");
	else
	{
		st = *(size_t*)(st + cur);
		cur += sizeof(st);
	}
	return *this;
}

BufferPacket & BufferPacket::operator >> (string&str)
{
	if(cur + str.size() > size)
		log -> _Error("BufferPacket >> string overflow\n");
	else
	{
		int index = 0;
		for(;index < str.size();index++)
			str[index] = data[cur + index];
		cur += str.size();
	}
	return *this;
}

void BufferPacket::write(const char * str, int len)
{
	if(cur + len > size) 
		log -> _Error("BufferPacket error write\n");
	else
	{
		memcpy(data + cur, str, len);
		cur += len;
	}
}


void BufferPacket::read(char * str, int len)
{
	if(cur + len > size)
		log -> _Error("BufferPacket >> char* overflow\n");
	else
	{
		memcpy(str,data + cur,len);
		cur += len;
	}
}

BufferPacket & BufferPacket::operator<<(const BufferPacket & packet)
{
	if(cur + packet.getSize() > size)
		log -> _Error("BufferPacket write packet overflow\n");
	else
	{
		memcpy(data + cur,(char*)&packet,packet.getSize());
		cur += packet.getSize();
	}

	return *this;
}

BufferPacket & BufferPacket::operator << (const char * str)
{
	if(cur + strlen(str) > size)
		log -> _Error("BufferPacket write char * overflow\n");
	else
	{
		memcpy(data + cur, str, strlen(str));
		cur += strlen(str);
	}

	return *this;
}

BufferPacket & BufferPacket::operator << (const Slice & slice)
{
	if(cur + slice.size() > size)
		log -> _Error("BufferPacket write slice overflow\n");
	else
	{
		memcpy(data + cur, slice.tochars(), slice.size());
		cur += slice.size();
	}

	return *this;
}

BufferPacket & BufferPacket::operator >> (Slice  & slice)
{
	if(cur + slice.size() > size)
		log -> _Error("BufferPacket >> slice overflow\n");
	else
	{
		Slice rs(data + cur, slice.size());
		slice = rs;
		cur += slice.size();
	}

	return *this;
}

BufferPacket & BufferPacket::operator >> (char * str)
{
	int index = cur;
	while(index < size)
	{
		if(data[index] == 0) break;
		index++;
	}

	memcpy(str, data + cur, index - cur + 1);
	
	cur = index + 1;
	
	return *this;
}
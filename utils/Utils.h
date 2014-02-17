#ifndef _UTILS_H
#define _UTILS_H

#include <fstream>
using namespace std;

#include <stdint.h>

#include <sys/types.h>
#include <unistd.h>

#include <assert.h>

inline uint64_t GetFileLen(FILE *file)
{
	uint64_t pos = ftell(file);
	fseek(file, 0, SEEK_END);
	uint64_t len = ftell(file);
	fseek(file, pos, SEEK_SET);
	
	return len;
} 

inline uint64_t GetFileLen(int fd)
{
	uint64_t pos = lseek(fd, 0, SEEK_CUR);
	uint64_t len = lseek(fd, 0, SEEK_END);

	assert(len != -1);
	assert(lseek(fd, 0, SEEK_SET) == 0);
	
	return len;
}

inline uint64_t GetFileLen(fstream &fs)
{
	uint64_t pos = fs.tellg();
	fs.seekg(0, ios_base::end);
	uint64_t len = fs.tellg();
	fs.seekg(0, ios_base::beg);
	return len;
}

#endif
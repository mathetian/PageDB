#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>

inline uint64_t GetFileLen(FILE *file)
{
	unsigned int pos = ftell(file);
	fseek(file, 0, SEEK_END);
	uint64_t len = ftell(file);
	fseek(file, pos, SEEK_SET);
	return len;
} 

#endif
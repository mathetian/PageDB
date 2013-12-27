#ifndef _OPTION_H
#define _OPTION_H

#define CUSTOMDB_W  0
#define CUSTOMDB_R  2
#define CUSTOMDB_C  4

#include <string>
using namespace std;

typedef struct{
	string fileName;
	unsigned int read_write : 2;
	unsigned int creat     : 1;
}FileOption;

typedef struct{
	FileOption foption;
}EnvOption;	

#define FIFO 0
#define  LRU 1

typedef struct{
	int cacheType;
	int sizeLimit;
}CacheOption;

#define EHASH 0
#define CHASH 1

typedef struct{
	int factoryType;
}FactoryOption;

typedef struct{
	int 		  logLevel;
	string 	      prefix;
	EnvOption     envOption;
	CacheOption   cacheOption;
	FactoryOption factoryOption;
}Options;

#endif
#ifndef _OPTION_H
#define _OPTION_H

#define CUSTOMDB_W  0
#define CUSTOMDB_R  2
#define CUSTOMDB_C  4

typedef struct{
	string fileName;
	int flag;
	int mode;
}FileOption;

typedef struct{
	FileOption foption;
}EnvOption;	

typedef struct{
	int type;
	int sizeLimit;
}CacheOption;

typedef struct{
	int type;
	int nsync;
}FactoryOption;

typedef struct{
	FileOption fileOption;
	EnvOption envOption;
	CacheOption CacheOption;
	FactoryOption factoryOption;
}Options;

#endif
#ifndef _OPTION_H
#define _OPTION_H
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
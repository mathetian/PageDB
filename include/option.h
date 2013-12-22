#ifndef _OPTION_H
#define _OPTION_H
typedef struct{
	string fileName;
	int flag;
	int mode;
}FileOption;

typedef struct{

};
class Options{
public:
private:
	int cacheLimit;
	const string fileName;
	int flag;
	int mode;
};
#endif
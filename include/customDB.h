#ifndef _CUSTOM_DB_H
#define _CUSTOM_DB_H
class CustomDB{
public:
	bool 	open(const string&filename,int flag,int mode);
	bool 	store(const string&key,const string&value);
	string  fetch(const string&key);
	int 	error();
private:
	int 	errorStatus;
};
#endif
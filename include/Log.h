#ifndef _LOG_H
#define _LOG_H

#include <string>
#include <iostream>
using namespace std;

#include <stdlib.h>

class Log{
public:
	static void w(const string&str)
	{
		cout<<str<<endl;
	}

	static void e(const string&str)
	{
		cout<<str<<endl;
		exit(1);
	}
};
#endif
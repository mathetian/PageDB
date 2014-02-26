#include "../extras/AIO.h"
using namespace utils;

#include <iostream>
using namespace std;

#include <string.h>
int main()
{
	AIOFile file;
	file.AIO_Open("hello3.txt");

	char buff[30] = "hello world";
	char buff2[30];memset(buff2,0,sizeof(buff2));
	file.AIO_Write(buff, -1, strlen(buff));
	//file.IO_Read(buff2, 0, strlen(buff));

	cout<<buff2<<endl;
	return 0;
}


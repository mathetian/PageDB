#include "../include/customDB.h"
#include "../include/option.h"
#include <iostream>
using namespace std;

int main()
{
	Options option;
	CustomDB*db = new CustomDB(option);
	db->open(option);
	db->put("hello","world");
	cout<<db->get("hello")<<end;
	delete db;
	
	return 0;
}
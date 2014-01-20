#include <iostream>
using namespace std;

#include "CustomDB.h"
#include "Option.h"

int main()
{
    Options option;
    
    CustomDB * db = new CustomDB;
    db -> open(option);
    printf("open successful\n");

/*{
    db -> put("hello","world");
    db -> put("hello1","world1");
    db -> put("hello12","world123");
}*/
{
    cout<< "get hello:"<< db -> get("hello") <<endl;
    cout<< "get hello1:"<< db -> get("hello1") <<endl;
    cout<< "get hello12:" << db -> get("hello12") <<endl;
}   
    db -> close();
    delete db;

    return 0;
}
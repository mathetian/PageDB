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
    db -> put("hello","world");
    cout<< db -> get("hello") <<endl;
    db -> put("hello1","world");
    cout<< db -> get("hello") <<endl;
    cout<< db -> get("hello1") <<endl;
    cout<< db -> remove("hello") << endl;
    cout << db -> get("hello") << endl;
    db -> put("hello12","world123");
    cout<< db -> get("hello12") <<endl;
    delete db;

    return 0;
}
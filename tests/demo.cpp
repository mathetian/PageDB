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
    cout<< db->get("hello") <<endl;
    delete db;

    return 0;
}
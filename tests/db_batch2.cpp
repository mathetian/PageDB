#include <iostream>
using namespace std;

/**Simple batch test**/

#include "CustomDB.h"
#include "Option.h"
#include "Batch.h"

int main()
{
    Options option;

    CustomDB * db = new CustomDB;
    db -> open(option);
    printf("open successful\n");

    {
        WriteBatch batch;
        batch.put("hello","world");
        batch.put("hello1","world1");
        batch.put("hello12","world123");

        db ->write(batch);
    }

    {
        cout<< "get hello:"<< db -> get("hello") <<endl;
        cout<< "get hello1:"<< db -> get("hello1") <<endl;
        cout<< "get hello12:" << db -> get("hello12") <<endl;
    }

    db -> close();
    delete db;

    return 0;
}
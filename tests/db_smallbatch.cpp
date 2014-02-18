#include <iostream>
using namespace std;

/**Simple batch test**/

#include "CustomDB.h"
#include "Option.h"
#include "Batch.h"
using namespace customdb;
using namespace utils;

#define EXPECT_EQ(a,b) assert(a == b)
#define EXPECT_EQ_S(a,b) assert(strcmp(a,b) == 1)

void RunTest1()
{
    Options option;

    CustomDB * db = new CustomDB;
    
    {
        db -> open(option);
        printf("open successful\n");
        
        WriteBatch batch;
        batch.put("hello","world");
        batch.put("hello1","world1");
        batch.put("hello12","world123");

        db ->write(&batch);
        db -> close();
    }

    {
        db -> open(option);
        printf("open successful\n");
        
        EXPECT_EQ(db->get("hello"), "world");
        EXPECT_EQ(db->get("hello1"), "world1");
        EXPECT_EQ(db->get("hello12"), "world123");

        db -> close();
    }

    delete db;
}

int main()
{
    RunTest1();
    printf("Passed All Tests\n");
    return 0;
}
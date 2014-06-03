#include <iostream>
using namespace std;

/**Simple batch test**/
#include "CustomDB.h"
#include "Option.h"
#include "Batch.h"
using namespace customdb;

#include "TestUtils.h"
using namespace utils;

class A { };

TEST(A, Test1)
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

        ASSERT_EQ(db->get("hello"), "world");
        ASSERT_EQ(db->get("hello1"), "world1");
        ASSERT_EQ(db->get("hello12"), "world123");

        db -> close();

        db -> destoryDB("demo");
    }

    delete db;
}

int main()
{
    RunAllTests();
    return 0;
}
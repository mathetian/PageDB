#include <iostream>
using namespace std;

#include "CustomDB.h"
#include "Option.h"
#include "Slice.h"
using namespace customdb;

#include "TestUtils.h"
using namespace utils;

class A { };

/**Basic Test for put and get**/
TEST(A, Test1)
{
    Options option;
    CustomDB * db = new CustomDB;

    {
        db -> open(option);
        printf("open successful RunTest1 in put\n");

        db -> put("hello","world");
        printf("put hello world\n");
        db -> put("hello1","world1");
        printf("put hello1 world1\n");
        db -> put("hello12","world123");
        printf("put hello12 world123\n");
        db -> close();
    }

    {
        db -> open(option);
        printf("open successful for RunTest1 in get\n");

        ASSERT_EQ(db -> get("hello"), Slice("world"));
        ASSERT_EQ(db -> get("hello1"), Slice("world1"));
        ASSERT_EQ(db -> get("hello12"), Slice("world123"));

        db -> close();
    }

    delete db;
}

/**Basic Test for compact, failed passed this Test**/
// TEST(A, Test2)
// {
//     Options option;
//     CustomDB * db = new CustomDB;

//     {
//         db -> open(option);
//         printf("open successful for RunTest2 in get\n");

//         ASSERT_EQ(db -> get("hello"), Slice("world"));
//         ASSERT_EQ(db -> get("hello1"), Slice("world1"));
//         ASSERT_EQ(db -> get("hello12"), Slice("world123"));

//         db -> close();
//     }

//     {
//         db -> open(option);
//         printf("open successful for RunTest2 in compact\n");
//         db -> compact();

//         db -> close();
//     }

//     {
//         db -> open(option);
//         printf("open successful for RunTest2 in test After compact\n");

//         ASSERT_EQ(db -> get("hello"), Slice("world"));
//         ASSERT_EQ(db -> get("hello1"), Slice("world1"));
//         ASSERT_EQ(db -> get("hello12"), Slice("world123"));

//         db -> close();

//         db -> destoryDB("demo");
//     }

//     delete db;
// }

int main()
{
    RunAllTests();
    return 0;
}
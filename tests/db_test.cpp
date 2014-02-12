#include <iostream>
using namespace std;

#include <assert.h>

#include "CustomDB.h"
#include "Option.h"
#include "Slice.h"

#define EXPECT_EQ(a,b) assert(a == b)
#define EXPECT_EQ_S(a,b) assert(strcmp(a,b) == 1); 

/**Basic Test for put and get**/
void RunTest1()
{
    Options option;
    CustomDB * db = new CustomDB;

    {  
        db -> open(option);
        printf("open successful RunTest1 in put\n");
        
        db -> put("hello","world");
        db -> put("hello1","world1");
        db -> put("hello12","world123");

        db -> close();
    }
    
    {
        db -> open(option);
        printf("open successful for RunTest1 in get\n");
        
        EXPECT_EQ(db -> get("hello"), Slice("world"));
        EXPECT_EQ(db -> get("hello1"), Slice("world1"));
        EXPECT_EQ(db -> get("hello12"), Slice("world123"));

        db -> close();
    }   

    delete db;
}

/**Basic Test for compact**/
void RunTest2()
{
    Options option;
    CustomDB * db = new CustomDB;

    {
        db -> open(option);
        printf("open successful for RunTest2 in get\n");
        
        EXPECT_EQ(db -> get("hello"), Slice("world"));
        EXPECT_EQ(db -> get("hello1"), Slice("world1"));
        EXPECT_EQ(db -> get("hello12"), Slice("world123"));

        db -> close();
    }   
    
    {
        db -> open(option);
        printf("open successful for RunTest2 in compact\n");
        db -> compact();

        EXPECT_EQ(db -> get("hello"), Slice("world"));
        EXPECT_EQ(db -> get("hello1"), Slice("world1"));
        EXPECT_EQ(db -> get("hello12"), Slice("world123"));

        db -> close();
    }

    delete db;
}

int main()
{
    RunTest1();
    RunTest2();
    printf("Passed All Tests\n");
    
    return 0;
}
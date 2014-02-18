#include <iostream>
#include <string>
using namespace std;

#include "../helpers/MemoryPool.h"
#include "../helpers/TimeStamp.h"

MemoryPool * mp    = NULL  ;
uint32_t TestCount = 50000 ;
uint32_t ArraySize = 1000  ;

class TestClassWithMP
{
public :
    TestClassWithMP()
    {
        memcpy(m_cMyArray, "Hello", 5);

        m_strMyString = "This is a small Test-String" ;
        m_iMyInt = 12345 ;

        m_fFloatValue = 23456.7890f ;
        m_fDoubleValue = 6789.012345 ;

        Next = this ;
    }

    virtual ~TestClassWithMP() {} ;

    void *operator new(std::size_t ObjectSize)
    {
        return mp -> getMemory(ObjectSize) ;
    }

    void operator delete(void *ptrObject, std::size_t ObjectSize)
    {
        if((mp -> freeMemory(ptrObject, ObjectSize)) == false)
            cerr<< "free Memory error" << endl;
    }
private :
    // Test-Data
    char m_cMyArray[25] ;
    unsigned char m_BigArray[10000] ;
    string m_strMyString ;
    int m_iMyInt ;
    TestClassWithMP *Next ;
    float m_fFloatValue ;
    double m_fDoubleValue ;
};

class TestClassWithHP
{
public :
    TestClassWithHP()
    {
        memcpy(m_cMyArray,"hello",5);
        m_strMyString = "This is a small Test-String" ;
        m_iMyInt = 12345 ;

        m_fFloatValue = 23456.7890f ;
        m_fDoubleValue = 6789.012345 ;

        Next = this ;
    }

    virtual ~TestClassWithHP() {} ;
private :
    char m_cMyArray[25] ;
    unsigned char m_BigArray[10000] ;
    string m_strMyString ;
    int m_iMyInt ;
    TestClassWithHP *Next ;
    float m_fFloatValue ;
    double m_fDoubleValue ;
};

void CreateGlobalMemPool()
{
    cout << "Creating MemoryPool....";
    mp = new MemoryPool() ;
    cout << "OK" << endl ;
}

void DestroyGlobalMemPool()
{
    cout << "Deleting MemPool....";
    if(mp) delete mp ;
    cout << "OK" << endl;
}

void TestAllocationSpeedClassMemPool()
{
    cout << "Allocating Memory (Object Size : " << sizeof(TestClassWithMP) << ")..." ;

    TimeStamp::StartTime();

    for(uint32_t j = 0; j < TestCount; j++)
    {
        TestClassWithMP *ptrTestClass = new TestClassWithMP ;
        delete ptrTestClass ;
    }

    cout << "OK" << endl ;
    const char * str = "Result for MemPool(Class TestClassWithMP) : ";
    TimeStamp::StopTime(str);
}

void TestAllocationSpeedClassHeap()
{
    cout << "Allocating Memory (Object Size : " << sizeof(TestClassWithHP) << ")..." ;

    for(uint32_t j = 0; j < TestCount; j++)
    {
        TestClassWithHP * ptrTestClass = new TestClassWithHP ;
        delete ptrTestClass ;
    }
    cout << "OK" << endl ;
    const char * str = "Result for Heap(Class TestClassWithHP)    : ";
    TimeStamp::StopTime(str);
}

void TestAllocationSpeedArrayMemPool()
{
    cout << "Allocating Memory (Object Size : " << ArraySize << ")..." ;

    TimeStamp::StartTime();

    for(uint32_t j = 0; j < TestCount; j++)
    {
        char *ptrArray = mp -> getMemory(ArraySize)  ;
        mp -> freeMemory(ptrArray, ArraySize) ;
    }
    cout << "OK" << endl ;

    const char * str = "Result for MemPool(Array-Test) : ";
    TimeStamp::StopTime(str);
}

void TestAllocationSpeedArrayHeap()
{
    cout << "Allocating Memory (Object Size : " << ArraySize << ")..." ;

    TimeStamp::StartTime();

    for(uint32_t j = 0; j < TestCount; j++)
    {
        char *ptrArray = new char[ArraySize]  ;
        delete [] ptrArray;
    }
    cout << "OK" << endl ;

    const char * str = "Result for Heap(Array-Test)    : ";
    TimeStamp::StopTime(str);
}

int main()
{
    cout << "MemoryPool Program started..." << endl ;
    CreateGlobalMemPool() ;

    TestAllocationSpeedClassMemPool() ;
    TestAllocationSpeedClassHeap() ;

    TestAllocationSpeedArrayMemPool() ;
    TestAllocationSpeedArrayHeap() ;

    DestroyGlobalMemPool() ;
    cout << "MemoryPool Program finished..." << endl ;
    return 0 ;
}

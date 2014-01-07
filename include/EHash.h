#ifndef _E_TABLE_H
#define _E_TABLE_H
#define PAGESIZE 25

#include <string>
#include <vector>
#include <fstream>
using namespace std;

#include "Factory.h"
#include "BufferPacket.h"

#define PAGESIZE  25
#define SINT      sizeof(int)
#define SPAGE     sizeof(Page)
#define SEEBLOCK  sizeof(EEmptyBlock)
#define SPELEMENT sizeof(PageElement)

typedef int (*HASH)(const string&key);

class EEmptyEle
{
public:
    EEmptyEle(): pos(-1), size(-1) { }

public:
    int   pos, size;
};

class EEmptyBlock
{
public:
    EEmptyBlock() : curNum(0), nextBlock(-1) { }
    ~EEmptyBlock() { }
    bool checkSuitable(int size, int & pos);
    EEmptyBlock split();
public:
    int      curNum;
    int      nextBlock;
    EEmptyEle eles[PAGESIZE];
};

class PageElement
{
public:
    PageElement(): hash_value(-1), data_pointer(-1), key_size(-1), data_size(-1){}
public:
    int   hash_value;
    int   data_pointer;
    int   key_size;
    int   data_size;
};

inline  ostream & operator << (ostream & os, PageElement & e)
{
    os << e.hash_value << " "<< e.data_pointer << " "<< e.key_size <<" "<<e.data_size<<endl;
    return os;
}

class ExtendibleHash;

class Page
{
public:
    Page(ExtendibleHash * eHash) : d(0),curNum(0) { this -> eHash = eHash; }
    ~ Page(){}

public:
    BufferPacket getPacket();
    void         setBucket(BufferPacket & packet);
    int          getSize() { return SINT*2 + sizeof(elements);}

    void         printAllEle()
    {
        int index = 0;
        for(;index < curNum;index++)
        {
            cout << elements[index] <<endl;
        }
        cout << "endls" << endl;
    }

private:
    bool   full() { return curNum > PAGESIZE; }
    bool   put(const string&key,const string&value, int hashVal);
    string get(const string&key, int hashVal);
    bool   remove(const string&key, int hashVal);

private:
    friend class ExtendibleHash;
    ExtendibleHash * eHash;

private:
    /**
       Asscoiate with the file,
       Use PAGESIZE + 5 to avoid some special situation
    **/
    int d, curNum;
    PageElement elements[PAGESIZE + 5];
};

class ExtendibleHash : public Factory
{
public:
    ExtendibleHash(HASH hashFunc = defaultHashFunc);
    ~ ExtendibleHash();

public:
    bool     put(const string&key,const string&value);
    string   get(const string&key);
    bool     remove(const string&key);
    bool     init(const string&filename);

private:
    void     cycle(int offset, int size);

private:
    void     writeToFile();
    void     readFromFile();
    int      findSuitable(int size);

private:
    HASH           hashFunc;
    Page   *       page;
    bool           updated;
    bool           eupdated;
    fstream        idxfs;
    fstream        datfs;
    friend  class  Page;

private:
    /**Need read from file**/
    int            gd, pn, fb;
    vector <int>   entries;
};

#endif
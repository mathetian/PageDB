#include <iostream>
using namespace std;

#include "CustomDB.h"
#include "Option.h"
#include <assert.h>

#define SIZE 1000

int main()
{
	Options option;
    	
    CustomDB * db = new CustomDB;
    db -> open(option);
    printf("open successful\n");

    for(int i=SIZE;i>=1;i--)
    {
        BufferPacket packet(sizeof(int));
        packet << i;
        Slice key(packet.getData(),sizeof(int));
        Slice value(packet.getData(),sizeof(int));
        cout << "begin put:" << i << endl;
        if(db -> put(key, value) == false)
            cout << "error put:" << i << endl;
    }

    db -> dump();
    db -> cleanCACHE();

    for(int i=SIZE;i>=1;i--)
    {
        BufferPacket packet(sizeof(int)); packet << i;
        
        Slice key(packet.getData(), sizeof(int));

        Slice value = db -> get(key);

        BufferPacket packet2(sizeof(int));
        
        packet2 << value; packet2.setBeg();
        
        int num = -1; packet2 >> num;
        if(i!=num)
            cout << i <<" "<<num <<" ) ";
    }
   //db -> dump();
    delete db;
}
    
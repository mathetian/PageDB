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

    for(int i=53;i>=1;i--)
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

/*    for(int i = 1;i <= 200;i++)
    {
        BufferPacket packet(sizeof(int)); packet << i;
        
        Slice key(packet.getData(), sizeof(int));

        Slice value = db -> get(key);

        BufferPacket packet2(sizeof(int));
        
        packet2 << value; packet2.setBeg();
       	
       	int num = -1; packet2 >> num;
       	cout << i << " " << num << endl;
    }*/


    delete db;
}
    
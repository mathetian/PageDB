#include <iostream>
using namespace std;

#include "CustomDB.h"
#include "Option.h"
#include "TimeStamp.h"

#include <assert.h>


#define SIZE 1000000

int main()
{
	Options option;
    CustomDB * db = new CustomDB;
    db -> open(option);
    printf("open successful\n");

    TimeStamp::StartTime();

    for(int i=SIZE;i>=1;i--)
    {
        BufferPacket packet(sizeof(int));
        packet << i;
        Slice key(packet.getData(),sizeof(int));
        Slice value(packet.getData(),sizeof(int));
        if(db -> put(key, value) == false) 
            cout << "error put:" << i << endl;
        if(i%10000==0) cout<<i<<endl;
    }

    TimeStamp::StopTime("PutTime: ");

    //db -> dump();
    db -> cleanCACHE();

    TimeStamp::StartTime();

   /* for(int i=SIZE;i>=1;i--)
    {
        BufferPacket packet(sizeof(int)); packet << i;
        
        Slice key(packet.getData(), sizeof(int));

        Slice value = db -> get(key);

        BufferPacket packet2(sizeof(int));
        
        packet2 << value; packet2.setBeg();
        
        int num = -1; packet2 >> num;
        if(i!=num)
            cout << i <<" "<<num <<" ) ";
    }*/

    TimeStamp::StopTime("GetTime(Without Cache): ");
    delete db;
}
    
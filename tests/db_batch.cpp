#include <iostream>
using namespace std;

#include "CustomDB.h"
#include "Option.h"
#include "TimeStamp.h"

#include <assert.h>


#define SIZE 1000000
#define BATCHSIZE 50000

int main()
{
	Options option; char str[256];
    CustomDB * db = new CustomDB;
    db -> open(option);
    printf("open successful\n");

    int round = SIZE/BATCHSIZE;
    if(SIZE % BATCHSIZE != 0) round++;

    for(int i = 0;i < round;i++)
    {
    	TimeStamp::StartTime();
    	
    	WriteBatch batch(BATCHSIZE);

    	for(int j = 0;j < BATCHSIZE;j++)
    	{
            int k = i*BATCHSIZE + j;
            if(k >= SIZE) break;

    		BufferPacket packet(sizeof(int));
	        packet << k;
	        Slice key(packet.getData(),sizeof(int));
	        Slice value(packet.getData(),sizeof(int));
    		batch.put(key, value);	
    	}
    	
    	db -> write(batch);

    	sprintf(str, "In round %d, PutTime: ", i);
    	TimeStamp::StopTime(str);
    }


    TimeStamp::StartTime();

    for(int i=SIZE-1;i>=0;i--)
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

    TimeStamp::StopTime("GetTime(Without Cache): ");
    delete db;
}
    
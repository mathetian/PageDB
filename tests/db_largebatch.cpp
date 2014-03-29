#include <iostream>
using namespace std;

#include "CustomDB.h"
#include "Option.h"
#include "TimeStamp.h"
#include "BufferPacket.h"
using namespace customdb;
using namespace utils;

#include <assert.h>

#define SIZE      100000
#define BATCHSIZE 1000

void RunTest1()
{
    Options option;
    char str[256];
    CustomDB * db = new CustomDB;

    TimeStamp total, part;

    {
        db -> open(option);
        printf("open successful\n");

        int round = (SIZE + BATCHSIZE - 1)/BATCHSIZE;

        total.StartTime();
        for(int i = 0; i < round; i++)
        {
            part.StartTime();

            WriteBatch batch(BATCHSIZE);

            for(int j = 0; j < BATCHSIZE; j++)
            {
                int k = i*BATCHSIZE + j;
                if(k >= SIZE) break;

                BufferPacket packet(sizeof(int));
                packet << k;
                Slice key(packet.getData(),sizeof(int));
                Slice value(packet.getData(),sizeof(int));
                batch.put(key, value);
            }

            db -> write(&batch);
            batch.clear();
            sprintf(str, "In round %d, PutTime: ", i);
            part.StopTime(str);
        }
        sprintf(str, "Total PutTime: ");
        total.StopTime(str);

        db -> close();
    }

    {
        db -> open(option);
        printf("open successful\n");

        total.StartTime();
        for(int i=SIZE-1; i>=0; i--)
        {
            BufferPacket packet(sizeof(int));
            packet << i;

            Slice key(packet.getData(), sizeof(int));

            Slice value = db -> get(key);

            BufferPacket packet2(sizeof(int));

            packet2 << value;
            packet2.setBeg();

            int num = -1;
            packet2 >> num;
            if(i!=num)
                cout << i <<" "<<num <<" ) ";
            //cout<<i<<endl;
        }
        total.StopTime("GetTime(Without Cache): ");

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
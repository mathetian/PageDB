#include "Slice.h"
#include "BufferPacket.h"

#include <assert.h>

#define EXPECT_EQ(a,b) assert(a == b)

void Test()
{
	BufferPacket packet(sizeof(int)); packet << 15545;
	Slice slice(packet.getData(), packet.getSize());
	
	EXPECT_EQ(slice.returnAsInt(), 15545);

	Slice slice2("");
	EXPECT_EQ(slice2.size(),0);
}

int main()
{
	Test();
}
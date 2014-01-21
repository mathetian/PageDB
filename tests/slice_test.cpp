#include "Slice.h"
#include "BufferPacket.h"

int main()
{
	BufferPacket packet(sizeof(int)); packet << 15545;
	Slice slice(packet.getData(), packet.getSize());
	
	slice.printAsInt();

	return 0;
}
#include "ChainHashDBImpl.h"

namespace customdb
{

ChainTable::ChainTable(ChainDB * db, int defaultFirstOffset) : db(db),\
    firstoffset(defaultFirstOffset) 
{ 

}

bool     ChainTable::put(const Slice & key,const Slice & value, uint32_t hashVal)
{
	if(check(key, hashVal) != 0)
        return false;

    ChainElement elem(firstoffset, key.size(), value.size(), hashVal);

    int offset = db -> findSuitableOffset(key.size() + value.size() + SELEM);

    db -> datfs.seekg(offset, ios_base::beg);

    BufferPacket packet(SELEM + key.size() + value.size());
    Slice slice((char*)&elem, SELEM); /**Need to record hashVal?**/
    packet << slice << key << value;

    db -> datfs.write(packet.getData(),packet.getSize());

    firstoffset = offset;

    return true;
}

Slice    ChainTable::get(const Slice & key, uint32_t hashVal)
{
	int offset = firstoffset;
    ChainElement elem;

    while(offset != -1)
    {
        db -> datfs.seekg(offset, ios_base::beg);
        db -> datfs.read ((char*)&elem, SELEM);

        offset = elem.nextOffset;

        if(elem.hashVal != hashVal || elem.keySize != key.size())
            continue;

        BufferPacket packet(elem.keySize + elem.valueSize);
        Slice key1(elem.keySize), value1(elem.valueSize);

        db -> datfs.read(packet.getData(), packet.getSize());

        packet >> key1 >> value1;

        if(key == key1) return value1;
    }
    return "";
}

bool   ChainTable::check(const Slice & key, uint32_t hashVal)
{
    int offset = firstoffset;
    ChainElement elem;

    while(offset != -1)
    {
        db -> datfs.seekg(offset, ios_base::beg);
        db -> datfs.read ((char*)&elem, SELEM);
        offset = elem.nextOffset;
        if(elem.hashVal != hashVal || elem.keySize != key.size())
            continue;

        BufferPacket packet(elem.keySize);
        db -> datfs.read(packet.getData(),packet.getSize());

        Slice slice(elem.keySize);
        if(key == slice) return true;
    }

    return false;
}

bool     ChainTable::remove(const Slice & key, uint32_t hashVal)
{
	int offset = firstoffset, oldoffset = offset;
    ChainElement elem;

    while(offset != -1)
    {
        db -> datfs.seekg(offset, ios_base::beg);
        db -> datfs.read ((char*)&elem, SELEM);

        oldoffset = offset;
        offset = elem.nextOffset;
        if(elem.hashVal != hashVal || elem.keySize != key.size())
            continue;

        BufferPacket packet(elem.keySize);
        db -> datfs.read(packet.getData(), packet.getSize());

        Slice slice(elem.keySize);
        packet >> slice;

        if(key == slice)
        {
            /**
            	Storage the emptry space into the the header linklist
            **/
            db -> recycle(oldoffset, SELEM + elem.keySize + elem.valueSize);
            return true;
        }
    }
    return false;
}

};
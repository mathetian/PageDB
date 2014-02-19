#include "ChainHashDBImpl.h"

namespace customdb
{

ChainElement::ChainElement(int nextOffset, int keySize, int valueSize, uint32_t hashVal) \
:
nextOffset(nextOffset), keySize(keySize), valueSize(valueSize), hashVal(hashVal)
{

}

};
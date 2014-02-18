#include "PageDBImpl.h"

namespace customdb{

PageElement::PageElement(): m_hashVal(0), \
    m_datPos(-1), m_keySize(-1), m_datSize(-1) { }

void PageElement::clear()
{
    m_hashVal = 0;
    m_datPos  = 0;
    m_keySize = 0;
    m_datSize = 0;
}

ostream & operator << (ostream & os, PageElement & e)
{
    os << e.m_hashVal << " "<< e.m_datPos << " "<< e.m_keySize <<" "<<e.m_datSize<<endl;
    return os;
}

};
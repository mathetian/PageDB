// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "PageDBImpl.h"

namespace customdb
{

PageElement::PageElement(): m_hashVal(0), \
    m_datPos(-1), m_keySize(-1), m_datSize(-1) { }

ostream & operator << (ostream & os, PageElement & e)
{
    os << e.m_hashVal << " "<< e.m_datPos << " "<< e.m_keySize << " " << e.m_datSize;
    return os;
}

};
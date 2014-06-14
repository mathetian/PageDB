// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "PageDBImpl.h"

namespace customdb
{

PageEmptyBlock::PageEmptyBlock() : m_curNum(0), m_nextBlock(-1)
{

}

/**
** Find suitable index
** Return true, if exist suitable element
** Return false, if not exist
**/
bool PageEmptyBlock::find(int size, int & pos)
{
    for(pos = m_curNum - 1; pos >= 0; pos--)
    {
        if(m_eles[pos].m_size > size)
            return true;
    }
    return false;
}

/**
** Split the EmptyBlock into two blocks
** Split it according the property of odd or even
**/
PageEmptyBlock PageEmptyBlock::split()
{
    PageEmptyBlock newblock;

    int cn1 = 0, cn2 = 0, index;

    for(index = 0; index < m_curNum; index++)
    {
        if(index & 1)
            newblock.m_eles[cn1++] = m_eles[index];
        else
            m_eles[cn2++] = m_eles[index];
    }

    newblock.m_curNum = cn1;
    this->m_curNum    = cn2;

    return newblock;
}

};
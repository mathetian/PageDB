// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _NON_COPYABLE_H
#define _NON_COPYABLE_H

/**
** Noncopyable is used to forbid copyable
**
** Noncopyable can't be used for STL containers.
**/
namespace utils
{

class Noncopyable
{
public:
	Noncopyable() { }
private:
	Noncopyable& operator=(const Noncopyable&);
	Noncopyable(const Noncopyable&);
};

};



#endif
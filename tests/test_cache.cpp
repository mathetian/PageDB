// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "Slice.h"
#include "TestUtils.h"
using namespace utils;

#include "Cache.h"
#include "LRUCache.h"
#include "FIFOCache.h"
#include "EmptyCache.h"
using namespace cache;

class A { };

TEST(A, EmptyCache)
{
    Cache *cache = new EmptyCache();

    cache -> put("hello1", "body1");
    cache -> put("hello2", "body2");

    ASSERT_EQ(cache -> get("hello1"), "");

    delete cache;
    cache = NULL;
}

TEST(A, FIFOCache)
{
    Cache *cache = new FIFOCache(2);
    cache -> put("hello1", "body1");
    cache -> put("hello2", "body2");
    cache -> put("hello3", "body3");

    /// Assert basic operation
    ASSERT_EQ(cache -> get("hello3"), "body3");

    /// Assert basic operation(after that, 43)
    cache -> put("hello4", "body4");
    ASSERT_EQ(cache -> get("hello2"), "");
    ASSERT_EQ(cache -> get("hello3"), "body3");

    /// Assert basic operation(after that, 54)
    cache -> put("hello3", "body31");
    cache -> put("hello5", "body5");

    ASSERT_EQ(cache -> get("hello3"), "");
    ASSERT_EQ(cache -> get("hello4"), "body4");

    delete cache;
    cache = NULL;
}

TEST(A, LRUCache)
{
    Cache *cache = new FIFOCache(2);
    cache -> put("hello1", "body1");
    cache -> put("hello2", "body2");
    cache -> put("hello3", "body3");

    /// Assert basic operation(after that, 32)
    ASSERT_EQ(cache -> get("hello3"), "body3");

    /// Assert basic operation(after that, 43)
    cache -> put("hello4", "body4");
    ASSERT_EQ(cache -> get("hello2"), "");

    /// Assert LRU(after that, 34)
    ASSERT_EQ(cache -> get("hello3"), "body3");

    /// Assert basic operation(after that, 53)
    cache -> put("hello3", "body31");
    cache -> put("hello5", "body5");

    ASSERT_EQ(cache -> get("hello3"), "body31");
    ASSERT_EQ(cache -> get("hello5"), "body5");
    ASSERT_EQ(cache -> get("hello4"), "");

    delete cache;
    cache = NULL;
}

int main()
{
    RunAllTests();
    return 0;
}
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _DB_BENCHMARK_H
#define _DB_BENCHMARK_H

#include <stdint.h>

namespace benchmarks
{

class Random
{
private:
    uint32_t seed_;

public:
    Random(uint32_t s) : seed_(s & 0x7fffffffu) {
        if (seed_ == 0 || seed_ == 2147483647L) {
            seed_ = 1;
        }
    }

    uint32_t Next() {
        static const uint32_t M = 2147483647L;   // 2^31-1
        static const uint64_t A = 16807;  // bits 14, 8, 7, 5, 2, 1, 0
        uint64_t product = seed_ * A;
        seed_ = static_cast<uint32_t>((product >> 31) + (product & M));
        if (seed_ > M) {
            seed_ -= M;
        }
        return seed_;
    }

    uint32_t Uniform(int n) {
        return Next() % n;
    }


    bool OneIn(int n) {
        return (Next() % n) == 0;
    }

    uint32_t Skewed(int max_log) {
        return Uniform(1 << Uniform(max_log + 1));
    }
};

class RandomGenerator
{
private:
    string data_;
    int pos_;

public:
    RandomGenerator() {
        Random rnd(301);
        std::string piece;

        while (data_.size() < 1048576) {
            uint32_t rr = rnd.Next();
            stringstream ss;
            ss << rr;
            piece = ss.str();
            data_.append(piece);
        }
        pos_ = 0;
    }

    Slice Generate(int len) {
        if (pos_ + len > data_.size()) {
            pos_ = 0;
            assert(len < data_.size());
        }
        pos_ += len;
        return Slice(data_.data() + pos_ - len, len);
    }
};

class Benchmark
{
public:
    Benchmark();
    ~Benchmark();

public:
    void Run();

private:
    void PrintHeader();
    void PrintWarnings();
    void PrintEnvironment();

private:
    void Open();
    void Close();

private:
    void RunBenchmark(Slice name, void (Benchmark::*method)());
    void FillSync();
    void FillBatch();

private:
    struct ThreadArg;
    static void* ThreadBody(void *arg);
    void* ThreadBatchBody(void *arg);
    void  FillParallelBatch();

private:
    void  ReadRandom();
    void  DestroyDB();

private:
    PageDB* db_;
    Options   option_;

    int num_;
    int value_size_;
    int key_size_;
    int reads_;
    int batchsize_;
    int num_cpus_;
};

};

#endif
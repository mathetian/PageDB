// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "Batch.h"
#include "Option.h"
#include "PageDB.h"
using namespace pagedb;

#include "TickTimer.h"
using namespace utils;

#include "db_benchmark.h"
using namespace benchmarks;

static const char* FLAGS_benchmarks = "fillsync, fillbatch, fillparallelbatch, readrandom";
//static const char* FLAGS_benchmarks = "fillbatch";
//static const char* FLAGS_benchmarks = "";

static int FLAGS_num = 1000000;

static int FLAGS_key_size = 16;

static int FLAGS_value_size = 100;

static int FLAGS_batch_size = 50000;

static bool FLAGS_use_existing_db = false;

static const char* FLAGS_db = NULL;

static const char* FLAGS_log =  NULL;

static Slice TrimSpace(Slice s)
{
    int start = 0;
    while (start < s.size() && isspace(s[start]))
    {
        start++;
    }

    int limit = s.size();
    while (limit > start && isspace(s[limit-1]))
    {
        limit--;
    }

    return Slice(s.c_str() + start, limit - start);
}

int kMajorVersion = 1;
int kMinorVersion = 1;

Benchmark::Benchmark() : db_(NULL), num_(FLAGS_num), key_size_(FLAGS_key_size),
    value_size_(FLAGS_value_size), reads_(FLAGS_num), batchsize_(FLAGS_batch_size)
{
    db_ =  new PageDB;
    assert(db_ != NULL);

    if(FLAGS_db)  option_.fileOption.fileName = FLAGS_db;
    if(FLAGS_log) option_.logOption.logPrefix = FLAGS_log;

    option_.logOption.logLevel   = Log::LOG_ERROR;
    option_.logOption.disabled   = true;
    option_.cacheOption.disabled = true;

    if (!FLAGS_use_existing_db)
    {
        DestroyDB();
    }
}

Benchmark::~Benchmark()
{
    db_ -> close();
    delete db_;
}


void Benchmark::Run()
{
    PrintHeader();

    const char* benchmarks = FLAGS_benchmarks;

    while (benchmarks != NULL)
    {
        const char* sep = strchr(benchmarks, ',');

        Slice name;

        if (sep == NULL)
        {
            name = TrimSpace(benchmarks);
            benchmarks = NULL;
        }
        else
        {
            name = TrimSpace(Slice(benchmarks, sep - benchmarks));
            benchmarks = sep + 1;
        }

        void (Benchmark::*method)() = NULL;

        bool fresh_db = false;

        if (name == Slice("fillsync"))
        {
            fresh_db = true;
            method = &Benchmark::FillSync;
        }
        else if (name == Slice("readrandom"))
        {
            method = &Benchmark::ReadRandom;
        }
        else if (name == Slice("fillbatch"))
        {
            fresh_db = true;
            method = &Benchmark::FillBatch;
        }
        else if(name == Slice("fillparallelbatch"))
        {
            fresh_db = true;
            method = &Benchmark::FillParallelBatch;
        }
        else
        {
            if (name != Slice())    // No error message for empty name
            {
                fprintf(stderr, "unknown benchmark '%s'\n", name.to_str().c_str());
            }
            continue;
        }

        if (fresh_db)
        {
            DestroyDB();
        }

        Open();

        if (method != NULL)
        {
            cout<<name<<endl;
            RunBenchmark(name, method);
        }

        Close();
    }
}

void Benchmark::PrintHeader()
{
    PrintEnvironment();

    fprintf(stdout, "Keys:       %d bytes each\n", key_size_);
    fprintf(stdout, "Values:     %d bytes each\n", value_size_);
    fprintf(stdout, "Entries:    %d\n", num_);

    double pageSize = PAGESIZE;

    int pagenum  = (num_/pageSize) * 1.7 * 1.6;
    int rawsize  = ((key_size_ + value_size_) * num_)/1000000;
    int filesize = (pagenum*1000 + (key_size_ + value_size_) * num_)/1000000;

    fprintf(stdout, "RawSize:    %d MB (estimated)\n", rawsize);
    fprintf(stdout, "FileSize:   %d MB (estimated)\n", filesize );

    PrintWarnings();

    fprintf(stdout, "------------------------------------------------\n");
}

void Benchmark::PrintWarnings()
{
    fprintf(stdout, "WARNING: Log is disabled  , for better benchmarks\n" );
    fprintf(stdout, "WARNING: Cache is disabled, for better benchmarks\n");
}

void Benchmark::PrintEnvironment()
{
    fprintf(stderr, "PageDB:    version %d.%d\n", kMajorVersion, kMinorVersion);

    time_t now = time(NULL);
    fprintf(stderr, "Date:       %s", ctime(&now));  // ctime() adds newline

    FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo != NULL)
    {
        char line[1000];
        num_cpus_ = 0;
        std::string cpu_type;
        std::string cache_size;
        while (fgets(line, sizeof(line), cpuinfo) != NULL)
        {
            const char* sep = strchr(line, ':');
            if (sep == NULL)
            {
                continue;
            }
            Slice key = TrimSpace(Slice(line, sep - 1 - line));
            Slice val = TrimSpace(Slice(sep + 1));
            if (key == "model name")
            {
                ++num_cpus_;
                cpu_type = val.to_str();
            }
            else if (key == "cache size")
            {
                cache_size = val.to_str();
            }
        }
        fclose(cpuinfo);
        fprintf(stderr, "CPU:        %d * %s\n", num_cpus_, cpu_type.c_str());
        fprintf(stderr, "CPUCache:   %s\n", cache_size.c_str());
    }
}

void Benchmark::Open()
{
    bool flag = db_ -> open(option_);

    if (!flag)
    {
        fprintf(stderr, "open error\n");
        exit(1);
    }
}

void Benchmark::Close()
{
    db_ -> close();
}

void Benchmark::RunBenchmark(Slice name, void (Benchmark::*method)())
{
    (this->*method)();
}

void Benchmark::FillSync()
{
    RandomGenerator gen;
    Random rnd(30);

    char key[100];

    Timer timer;
    timer.Start();

    char format[10];
    memset(format, 0, 10);
    snprintf(format, sizeof(format), "%%0%dd\n", key_size_);

    for(int i = 0; i < num_; i++)
    {
        if(i%10000 == 0)
            printf("WriteRandom %d\n", i);

        int k = rnd.Next() & ((1<<25)-1);
        snprintf(key, sizeof(key), format, k);

        db_->put(key, gen.Generate(FLAGS_value_size));
    }

    timer.Stop();
    timer.Print("WriteRandom spend time: ");
}

void Benchmark::FillBatch()
{
    RandomGenerator gen;
    Random rnd(31);

    char key[100], str[256];

    char format[10];
    memset(format, 0, 10);
    snprintf(format, sizeof(format), "%%0%dd\n", key_size_);

    int batchNum = (num_ + batchsize_ - 1)/batchsize_;

    Timer total, part;
    total.Start();

    for(int i = 0; i < batchNum; i++)
    {
        part.Start();

        WriteBatch batch(batchsize_);

        for(int j = 0; j < batchsize_; j++)
        {
            int k = rnd.Next() & ((1<<28)-1);
            snprintf(key, sizeof(key), format, k);

            batch.put(key, gen.Generate(value_size_));
        }

        db_ -> put(&batch);

        sprintf(str, "In round %d, PutTime: ", i);

        part.Stop();
        part.Print(str);
    }

    total.Stop();
    total.Print("WriteBatch Total Time: ");
}

struct Benchmark::ThreadArg
{
    Benchmark* bm;
    int        thrid;
    void* (Benchmark::*method)(void*);
};

void* Benchmark::ThreadBody(void *arg)
{
    ThreadArg *args = (ThreadArg*)arg;
    (args->bm->*(args->method))(&(args->thrid));
}

void* Benchmark::ThreadBatchBody(void *arg)
{
    int thrid = *(int*)arg;

    RandomGenerator gen;
    Random rnd(33 + thrid);

    char key[100], str[256];

    char format[10];
    memset(format, 0, 10);
    snprintf(format, sizeof(format), "%%0%dd\n", key_size_);

    int eachNum =  (num_    + num_cpus_  - 1)/num_cpus_;
    int batchNum = (eachNum + batchsize_ - 1)/batchsize_;

    printf("Thread %d start, batchNum %d\n", thrid, batchNum);

    Timer total;

    total.Start();

    for(int i = 0; i < batchNum; i++)
    {
        WriteBatch batch(batchsize_);

        for(int j = 0; j < batchsize_; j++)
        {
            int k = rnd.Next() & ((1<<25)-1);
            snprintf(key, sizeof(key), format, k);

            batch.put(key, gen.Generate(value_size_));
        }

        db_ -> put(&batch);
        printf("thread %d finished round %d\n", thrid, i);
    }

    sprintf(str, "Thread %d finished: ", thrid);

    total.Stop();
    total.Print(str);
}

void Benchmark::FillParallelBatch()
{
    vector<Thread*> thrs;
    ThreadArg *args = new ThreadArg[num_cpus_];

    for(uint64_t i = 0; i < num_cpus_; i++)
    {
        args[i].method = &Benchmark::ThreadBatchBody;
        args[i].bm     = this;
        args[i].thrid  = i;
        thrs.push_back(new Thread(ThreadBody, &(args[i])));
        thrs[thrs.size()-1] -> run();
    }

    for(uint64_t i = 0; i < num_cpus_; i++)
    {
        thrs[i] -> join();
    }

    delete [] args;
    args = NULL;

    for(uint64_t i = 0; i < num_cpus_; i++)
    {
        delete thrs[i];
        thrs[i] = NULL;
    }
}

void Benchmark::ReadRandom()
{
    int found = 0;

    Random rnd(40);
    char  key[100];

    char format[10];
    memset(format, 0, 10);
    snprintf(format, sizeof(format), "%%0%dd\n", key_size_);

    Timer timer;
    timer.Start();

    for (int i = 0; i < reads_; i++)
    {
        if(i%10000==0)
            printf("ReadRandom %d\n", i);

        const int k = rnd.Next() & ((1<<25)-1);
        snprintf(key, sizeof(key), format, k);

        Slice value;

        value = db_ -> get(key);

        if(value.size() != 0) found++;
    }

    char msg[100];
    snprintf(msg, sizeof(msg), "(%d of %d found)", found, num_);
    puts(msg);

    timer.Stop();
    timer.Print("ReadRandom spend time: ");
}

void Benchmark::DestroyDB()
{
    db_ -> destoryDB(option_.fileOption.fileName);
}

int main(int argc, char** argv)
{
    Benchmark benchmark;
    benchmark.Run();

    return 0;
}

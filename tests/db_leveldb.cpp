// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

//This file is mainly copied from LevelDB's benchmark file.

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
using namespace std;

#include "db_leveldb.h"

#include "Batch.h"
#include "Option.h"
#include "CustomDB.h"
#include "TimeStamp.h"
using namespace customdb;
using namespace utils;

#include <string.h>

static const char* FLAGS_benchmarks = "fillparallel,fillthreadbatch,fillbatch,fillrandom,readrandom";

// Number of key/values to place in database
static int FLAGS_num = 1000000;

static int FLAGS_value_size = 100;

static int FLAGS_batch_size = 50000;

static bool FLAGS_use_existing_db = false;

static const char* FLAGS_db = NULL;

static const char* FLAGS_log =  NULL;

class RandomGenerator
{
private:
    std::string data_;
    int pos_;

public:
    RandomGenerator()
    {
        // We use a limited amount of data over and over again and ensure
        // that it is larger than the compression window (32KB), and also
        // large enough to serve all typical value sizes we want to write.
        Random rnd(301);
        std::string piece;

        while (data_.size() < 1048576)
        {
            uint32_t rr = rnd.Next();
            stringstream ss;
            ss << rr;
            piece = ss.str();
            data_.append(piece);
        }
        pos_ = 0;
    }

    Slice Generate(int len)
    {
        if (pos_ + len > data_.size())
        {
            pos_ = 0;
            assert(len < data_.size());
        }
        pos_ += len;
        return Slice(data_.data() + pos_ - len, len);
    }
};

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

static void AppendWithSpace(std::string* str, Slice msg)
{
    if (msg.empty()) return;
    if (!str->empty())
    {
        str->push_back(' ');
    }
    str->append(msg.c_str(), msg.size());
}

int kMajorVersion = 1;
int kMinorVersion = 0;

class Benchmark
{
private:
    CustomDB* db_;
    Options option_;

    int num_;
    int value_size_;
    int reads_;
    int batchsize_;

    void PrintHeader()
    {
        const int kKeySize = 16;
        PrintEnvironment();
        fprintf(stdout, "Keys:       %d bytes each\n", kKeySize);
        fprintf(stdout, "Values:     %d bytes each\n", FLAGS_value_size);
        fprintf(stdout, "Entries:    %d\n", num_);
        fprintf(stdout, "RawSize:    %.1f MB (estimated)\n",
                ((static_cast<int64_t>(kKeySize + FLAGS_value_size) * num_) / 1048576.0));
        fprintf(stdout, "FileSize:   %.1f MB (estimated)\n",
                (((kKeySize + FLAGS_value_size) * num_)
                 / 1048576.0));
        PrintWarnings();
        fprintf(stdout, "------------------------------------------------\n");
    }

    void PrintWarnings()
    {
        fprintf(stdout, "WARNING: Log is disabled, for better benchmarks\n" );
    }

    void PrintEnvironment()
    {
        fprintf(stderr, "CustomDB:    version %d.%d\n", kMajorVersion, kMinorVersion);

        time_t now = time(NULL);
        fprintf(stderr, "Date:       %s", ctime(&now));  // ctime() adds newline

        FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
        if (cpuinfo != NULL)
        {
            char line[1000];
            num_cpus = 0;
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
                    ++num_cpus;
                    cpu_type = val.toString();
                }
                else if (key == "cache size")
                {
                    cache_size = val.toString();
                }
            }
            fclose(cpuinfo);
            fprintf(stderr, "CPU:        %d * %s\n", num_cpus, cpu_type.c_str());
            fprintf(stderr, "CPUCache:   %s\n", cache_size.c_str());
        }
    }

public:
    Benchmark() : db_(NULL), num_(FLAGS_num), value_size_(FLAGS_value_size), \
        reads_(FLAGS_num), batchsize_(FLAGS_batch_size)
    {
        db_ =  new CustomDB;
        assert(db_ != NULL);

        if(FLAGS_db) option_.fileOption.fileName = FLAGS_db;
        if(FLAGS_log) option_.logOption.logPrefix = FLAGS_log;
        
        option_.logOption.logLevel = LOG_ERROR;
        option_.logOption.disabled = true;

        if (!FLAGS_use_existing_db)
        {
            DestroyDB();
        }
    }

    ~Benchmark()
    {
        delete db_;
    }

    void Run()
    {
        PrintHeader();

        const char* benchmarks = FLAGS_benchmarks;

        while (benchmarks != NULL)
        {
            const char* sep = strchr(benchmarks, ',');
            Slice name;
            if (sep == NULL)
            {
                name = benchmarks;
                benchmarks = NULL;
            }
            else
            {
                name = Slice(benchmarks, sep - benchmarks);
                benchmarks = sep + 1;
            }

            // Reset parameters that may be overriddden bwlow
            num_ = FLAGS_num;
            reads_ = FLAGS_num;
            value_size_ = FLAGS_value_size;

            void (Benchmark::*method)() = NULL;

            bool fresh_db = false;

            if (name == Slice("fillrandom"))
            {
                fresh_db = true;
                method = &Benchmark::WriteRandom;
            }
            else if (name == Slice("readrandom"))
            {
                method = &Benchmark::ReadRandom;
            }
            else if (name == Slice("fillbatch"))
            {
                fresh_db = true;
                method = &Benchmark::WriteBatch2;
            }
            else if (name == Slice("fillthreadbatch"))
            {
                fresh_db = true;
                method = &Benchmark::WriteThreadBatch;
            }
            else if(name == Slice("fillparallel"))
            {
                fresh_db = true;
                method = &Benchmark::WriteParallelBatch;
            }
            else
            {
                if (name != Slice())    // No error message for empty name
                {
                    fprintf(stderr, "unknown benchmark '%s'\n", name.toString().c_str());
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

            db_ -> close();
        }
    }

private:
    void RunBenchmark(Slice name, void (Benchmark::*method)())
    {
        (this->*method)();
    }

    void Open()
    {
        int flag = db_ -> open(option_);

        if (!flag)
        {
            fprintf(stderr, "open error\n");
            exit(1);
        }
    }

    void WriteRandom()
    {
        RandomGenerator gen;

        int64_t bytes = 0;

        Random rnd(31);
        char key1[100];
        
        TimeStamp m_tms;
        m_tms.StartTime();

        for(int i = 0; i < num_; i++)
        {
            if(i%10000 == 0)
                printf("WriteRandom %d\n", i);
            
            int k = rnd.Next() & ((1<<25)-1);
            snprintf(key1, sizeof(key1), "%016d", k);
            Slice key(key1,16);
            Slice value(gen.Generate(FLAGS_value_size));
            db_->put(key, value);
        }

        m_tms.StopTime("WriteRandom spend time:");
    }

    void WriteBatch2()
    {
        int batchNum = (num_ + batchsize_ - 1)/batchsize_;
        
        RandomGenerator gen;

        Random rnd(32);
        char key1[100]; char str[256];
        TimeStamp total, part;

        total.StartTime();

        for(int i = 0;i < batchNum;i++)
        {
            part.StartTime();
            
            WriteBatch batch(batchsize_);

            for(int j = 0;j < batchsize_;j++)
            {
                int k = rnd.Next() & ((1<<25)-1);
                snprintf(key1, sizeof(key1), "%016d", k);
                Slice key(key1,16);
                Slice value(gen.Generate(FLAGS_value_size));
                batch.put(key, value);
            }
            
            db_ -> write(&batch);
            
            sprintf(str, "In round %d, PutTime: ", i);
            part.StopTime(str);
        }

        total.StopTime("WriteBatch Total Time: ");
    }
  
  struct ThreadArg {
    Benchmark* bm;
    int thrid;
    void* (Benchmark::*method)(int);
  };

    static void* ThreadBody(void *arg)
    {
        ThreadArg *thrargs = (ThreadArg*)arg;
        (thrargs->bm->*(thrargs->method))(thrargs->thrid);
    }

    void* ThreadBatchBody(int thrid)
    {
        RandomGenerator gen;
        char str[256];Random rnd(32);
        char key1[100];

        int eachNum = (num_ + num_cpus - 1)/num_cpus;
        int batchNum = (eachNum + batchsize_ - 1)/batchsize_;
        printf("Thread %d start, batchNum %d\n", thrid, batchNum);

        TimeStamp m_tms;
        
        m_tms.StartTime();
        for(int i = 0;i < batchNum;i++)
        {
            WriteBatch batch(batchsize_);
            
            for(int j = 0;j < batchsize_;j++)
            {
                int k = rnd.Next() & ((1<<25)-1);
                snprintf(key1, sizeof(key1), "%016d", k);
                Slice key(key1,16);
                Slice value(gen.Generate(FLAGS_value_size));
                batch.put(key, value);
            }
            db_ -> tWrite(&batch);
            printf("thread %d finished round %d\n", thrid, i);
        }
        sprintf(str, "Thread %d finished: ", thrid);
        m_tms.StopTime(str);
    }

    void* ThreadParallelBatchBody(int thrid)
    {
        RandomGenerator gen;
        char str[256];Random rnd(32);
        char key1[100];

        int eachNum = (num_ + num_cpus - 1)/num_cpus;
        int batchNum = (eachNum + batchsize_ - 1)/batchsize_;
        printf("Thread %d start, batchNum %d\n", thrid, batchNum);

        TimeStamp m_tms;
        
        m_tms.StartTime();
        for(int i = 0;i < batchNum;i++)
        {
            WriteBatch batch(batchsize_);
            
            for(int j = 0;j < batchsize_;j++)
            {
                int k = rnd.Next() & ((1<<25)-1);
                snprintf(key1, sizeof(key1), "%016d", k);
                Slice key(key1,16);
                Slice value(gen.Generate(FLAGS_value_size));
                batch.put(key, value);
            }
            db_ -> runBatchParallel(&batch);
            printf("thread %d finished round %d\n", thrid, i);
        }
        sprintf(str, "Thread %d finished: ", thrid);
        m_tms.StopTime(str);
    }

    void WriteThreadBatch()
    {
        vector<Thread> thrs;
        ThreadArg *args = new ThreadArg[num_cpus];
        
        for(uint64_t i = 0;i < num_cpus;i++)
        {
            args[i].method = &Benchmark::ThreadBatchBody;
            args[i].bm     = this;
            args[i].thrid  = i;
            thrs.push_back(Thread(ThreadBody, &(args[i])));
            thrs[thrs.size()-1].run();
        }

        for(uint64_t i = 0;i < num_cpus;i++)
        {
            thrs[i].join();
        }

        delete [] args; args = NULL;
    }

    void WriteParallelBatch()
    {
        vector<Thread> thrs;
        ThreadArg *args = new ThreadArg[num_cpus];
        
        for(uint64_t i = 0;i < num_cpus;i++)
        {
            args[i].method = &Benchmark::ThreadParallelBatchBody;
            args[i].bm     = this;
            args[i].thrid  = i;
            thrs.push_back(Thread(ThreadBody, &(args[i])));
            thrs[thrs.size()-1].run();
        }

        for(uint64_t i = 0;i < num_cpus;i++)
        {
            thrs[i].join();
        }

        delete [] args; args = NULL;
    }

    void ReadRandom()
    {
        int found = 0;

        Random rnd(32);

        char key1[100];
        
        TimeStamp m_tms;
        m_tms.StartTime();

        for (int i = 0; i < reads_; i++)
        {
            if(i%10000==0) 
                printf("ReadRandom %d\n", i);
            
            const int k = rnd.Next() & ((1<<25)-1);
            snprintf(key1, sizeof(key1), "%016d", k);
            
            Slice key(key1,16);
            Slice value;
            
            value = db_ -> get(key);
            
            if(value.size()!=0) found++;
        }

        char msg[100]; 
        snprintf(msg, sizeof(msg), "(%d of %d found)", found, num_);
        cout<<msg<<endl;
        m_tms.StopTime("ReadRandom spend time:");
    }

    void DestroyDB()
    {
        const char * filename = option_.fileOption.fileName;
        string sfilename(filename, filename + strlen(filename));
        string idxName = sfilename + ".idx";
        string datName = sfilename +  ".dat";

        idxName = "rm " + idxName;
        datName = "rm " + datName;
        system(idxName.c_str());
        system(datName.c_str());
    }

private:
    int num_cpus;
};

int main(int argc, char** argv)
{
    std::string default_db_path;

    for (int i = 1; i < argc; i++)
    {
        double d;
        int n;
        char junk;
        string str = argv[i];

        if (str.find("--benchmarks="))
        {
            FLAGS_benchmarks = argv[i] + strlen("--benchmarks=");
        }
        else if (sscanf(argv[i], "--use_existing_db=%d%c", &n, &junk) == 1 && (n == 0 || n == 1))
        {
            FLAGS_use_existing_db = n;
        }
        else if (sscanf(argv[i], "--num=%d%c", &n, &junk) == 1)
        {
            FLAGS_num = n;
        }
        else if (sscanf(argv[i], "--value_size=%d%c", &n, &junk) == 1)
        {
            FLAGS_value_size = n;
        }
        else if (strncmp(argv[i], "--db=", 5) == 0)
        {
            FLAGS_db = argv[i] + 5;
        }
        else
        {
            fprintf(stderr, "Invalid flag '%s'\n", argv[i]);
            exit(1);
        }
    }

    if (FLAGS_db == NULL)
    {
        default_db_path = "dbbench";
        FLAGS_db = default_db_path.c_str();
    }

    Benchmark benchmark;
    benchmark.Run();
    return 0;
}

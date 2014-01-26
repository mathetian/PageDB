// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

//This file is mainly copied from LevelDB's benchmark file.

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
using namespace std;

#include "Random.h"

#include "Option.h"
#include "CustomDB.h"
#include "TimeStamp.h"

static const char* FLAGS_benchmarks = "fillrandom,readrandom";

// Number of key/values to place in database
static int FLAGS_num = 1000000;

// Size of each value
static int FLAGS_value_size = 100;

// Number of bytes to buffer in memtable before compacting
// (initialized to default value by "main")
static int FLAGS_write_buffer_size = 0;

// Number of bytes to use as a cache of uncompressed data.
// Negative means use default settings.
static int FLAGS_cache_size = 0;

// If true, do not destroy the existing database.  If you set this
// flag and also specify a benchmark that wants a fresh database, that
// benchmark will fail.
static bool FLAGS_use_existing_db = false;

// Use the db with the following name.
static const char* FLAGS_db = NULL;

static const char* FLAGS_log =  NULL;

class RandomGenerator {
 private:
  std::string data_;
  int pos_;

 public:
  RandomGenerator() {
    // We use a limited amount of data over and over again and ensure
    // that it is larger than the compression window (32KB), and also
    // large enough to serve all typical value sizes we want to write.
    Random rnd(301);
    std::string piece;
    while (data_.size() < 1048576) {
      //Todo list
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

static Slice TrimSpace(Slice s) {
  int start = 0;
  while (start < s.size() && isspace(s[start])) {
    start++;
  }
  int limit = s.size();
  while (limit > start && isspace(s[limit-1])) {
    limit--;
  }
  return Slice(s.c_str() + start, limit - start);
}

static void AppendWithSpace(std::string* str, Slice msg) {
  if (msg.empty()) return;
  if (!str->empty()) {
    str->push_back(' ');
  }
  str->append(msg.c_str(), msg.size());
}

int kMajorVersion = 0;
int kMinorVersion = 1;

class Stats {
 private:
  double start_;
  double finish_;
  double seconds_;
  int done_;
  int next_report_;
  int64_t bytes_;
  double last_op_finish_;
  std::string message_;

 public:
  Stats() { Start(); }

  void Start() {
    next_report_ = 100;
    last_op_finish_ = start_;
    done_ = 0;
    bytes_ = 0;
    seconds_ = 0;
    //start_ = Env::Default()->NowMicros();
    start_ = TimeStamp::GetCurTimeAsDouble();
    finish_ = start_;
    message_.clear();
  }

  void Merge(const Stats& other) {
    done_ += other.done_;
    bytes_ += other.bytes_;
    seconds_ += other.seconds_;
    if (other.start_ < start_) start_ = other.start_;
    if (other.finish_ > finish_) finish_ = other.finish_;

    // Just keep the messages from one thread
    if (message_.empty()) message_ = other.message_;
  }

  void Stop() {
    finish_ = TimeStamp::GetCurTimeAsDouble();
    seconds_ = (finish_ - start_) * 1e-6;
  }

  void AddMessage(Slice msg) {
    AppendWithSpace(&message_, msg);
  }

  void FinishedSingleOp() {

    done_++;
    if (done_ >= next_report_) {
      if      (next_report_ < 1000)   next_report_ += 100;
      else if (next_report_ < 5000)   next_report_ += 500;
      else if (next_report_ < 10000)  next_report_ += 1000;
      else if (next_report_ < 50000)  next_report_ += 5000;
      else if (next_report_ < 100000) next_report_ += 10000;
      else if (next_report_ < 500000) next_report_ += 50000;
      else                            next_report_ += 100000;
      fprintf(stderr, "... finished %d ops%30s\r", done_, "");
      fflush(stderr);
    }
  }

  void AddBytes(int64_t n) {
    bytes_ += n;
  }

  void Report(const Slice& name) {
    // Pretend at least one op was done in case we are running a benchmark
    // that does not call FinishedSingleOp().
    if (done_ < 1) done_ = 1;

    std::string extra;
    if (bytes_ > 0) {
      // Rate is computed on actual elapsed time, not the sum of per-thread
      // elapsed times.
      double elapsed = (finish_ - start_) * 1e-6;
      char rate[100];
      snprintf(rate, sizeof(rate), "%6.1f MB/s",
               (bytes_ / 1048576.0) / elapsed);
      extra = rate;
    }
    AppendWithSpace(&extra, message_);

    fprintf(stdout, "%-12s : %11.3f micros/op;%s%s\n",
            name.c_str(),
            seconds_ * 1e6 / done_,
            (extra.empty() ? "" : " "),
            extra.c_str());
    fflush(stdout);
  }
};

class Benchmark {
 private:
  CustomDB* db_;
  Options option_;

  int num_;
  int value_size_;
  int reads_;

  void PrintHeader() {
    const int kKeySize = 16;
    PrintEnvironment();
    fprintf(stdout, "Keys:       %d bytes each\n", kKeySize);
    fprintf(stdout, "Values:     %d bytes each (%d bytes after compression)\n",
            FLAGS_value_size,
            static_cast<int>(FLAGS_value_size + 0.5));
    fprintf(stdout, "Entries:    %d\n", num_);
    fprintf(stdout, "RawSize:    %.1f MB (estimated)\n",
            ((static_cast<int64_t>(kKeySize + FLAGS_value_size) * num_)
             / 1048576.0));
    fprintf(stdout, "FileSize:   %.1f MB (estimated)\n",
            (((kKeySize + FLAGS_value_size) * num_)
             / 1048576.0));
    PrintWarnings();
    fprintf(stdout, "------------------------------------------------\n");
  }

  void PrintWarnings() {
#if defined(__GNUC__) && !defined(__OPTIMIZE__)
    fprintf(stdout,
            "WARNING: Optimization is disabled: benchmarks unnecessarily slow\n"
            );
#endif
#ifndef NDEBUG
    fprintf(stdout,
            "WARNING: Assertions are enabled; benchmarks unnecessarily slow\n");
#endif
  }

  void PrintEnvironment() {
    fprintf(stderr, "CustomDB:    version %d.%d\n",
            kMajorVersion, kMinorVersion);

#if defined(__linux)
    time_t now = time(NULL);
    fprintf(stderr, "Date:       %s", ctime(&now));  // ctime() adds newline

    FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo != NULL) {
      char line[1000];
      int num_cpus = 0;
      std::string cpu_type;
      std::string cache_size;
      while (fgets(line, sizeof(line), cpuinfo) != NULL) {
        const char* sep = strchr(line, ':');
        if (sep == NULL) {
          continue;
        }
        Slice key = TrimSpace(Slice(line, sep - 1 - line));
        Slice val = TrimSpace(Slice(sep + 1));
        if (key == "model name") {
          ++num_cpus;
          cpu_type = val.toString();
        } else if (key == "cache size") {
          cache_size = val.toString();
        }
      }
      fclose(cpuinfo);
      fprintf(stderr, "CPU:        %d * %s\n", num_cpus, cpu_type.c_str());
      fprintf(stderr, "CPUCache:   %s\n", cache_size.c_str());
    }
#endif
  }

 public:
  Benchmark() : db_(NULL), num_(FLAGS_num), value_size_(FLAGS_value_size), reads_(FLAGS_num)
  {
    db_ =  new CustomDB;    
    assert(db_ != NULL);
    
    if(FLAGS_db) option_.fileOption.fileName = FLAGS_db;
    if(FLAGS_log) option_.logOption.logPrefix = FLAGS_log;
    if(FLAGS_cache_size != 0) option_.cacheOption.cacheLimitInMB = FLAGS_cache_size;
    option_.logOption.logLevel = LOG_FATAL;
    if (!FLAGS_use_existing_db) {
      DestroyDB();
    }
  }

  ~Benchmark() {
    delete db_;
  }

  void Run() {
    PrintHeader();
    Open();

    const char* benchmarks = FLAGS_benchmarks;
    while (benchmarks != NULL) {
      const char* sep = strchr(benchmarks, ',');
      Slice name;
      if (sep == NULL) {
        name = benchmarks;
        benchmarks = NULL;
      } else {
        name = Slice(benchmarks, sep - benchmarks);
        benchmarks = sep + 1;
      }

      // Reset parameters that may be overriddden bwlow
      num_ = FLAGS_num;
      reads_ = FLAGS_num;
      value_size_ = FLAGS_value_size;

      void (Benchmark::*method)() = NULL;
      
      bool fresh_db = false;

      if (name == Slice("fillrandom")) {
        fresh_db = true;
        method = &Benchmark::WriteRandom;
      }else if (name == Slice("readrandom")) {
        method = &Benchmark::ReadRandom;
      }else {
        if (name != Slice()) {  // No error message for empty name
          fprintf(stderr, "unknown benchmark '%s'\n", name.toString().c_str());
        }
      }

      if (fresh_db) {
        if (FLAGS_use_existing_db) {
          fprintf(stdout, "%-12s : skipped (--use_existing_db is true)\n",
                  name.toString().c_str());
          method = NULL;
        } else {
          DestroyDB();
          Open();
        }
      }

      if (method != NULL) {
        cout<<name<<endl;
        RunBenchmark(name, method);
      }

      db_ -> close();
    }
  }

 private:
  void RunBenchmark(Slice name, void (Benchmark::*method)()){
     (this->*method)();
  }

  void Open() {
    if(db_ == NULL) db_ = new CustomDB;
    assert(db_ != NULL);
    int flag = db_ -> open(option_);
    
    if (!flag) {
      fprintf(stderr, "open error\n");
      exit(1);
    }
  }

  void WriteRandom() {
    DoWrite(false);
  }

  void DoWrite(bool seq) {
    if (num_ != FLAGS_num) {
      char msg[100];
      snprintf(msg, sizeof(msg), "(%d ops)", num_);
    }
    RandomGenerator gen;

    int64_t bytes = 0;
    Random rand1(31);
    
    for(int i = 0;i < num_;i++)
    {
      if(i%10000 == 0) cout<<i<<endl;
      int k = rand1.Next() & ((1<<16)-1);
      char key1[100];
      snprintf(key1, sizeof(key1), "%016d", k);

      Slice key(key1,16);
      Slice value(gen.Generate(FLAGS_value_size));
      db_->put(key,value);
    }
  }

  void ReadRandom() {
    int found = 0;
    
    Random rnd(31);

    for (int i = 0; i < reads_; i++) {
      char key1[100];
      const int k = rnd.Next() & ((1<<16)-1);
      snprintf(key1, sizeof(key1), "%016d", k);
      Slice key(key1,16);Slice value;
      value = db_ -> get(key);
      if(value.size()!=0) found++;
    }

    char msg[100];
    snprintf(msg, sizeof(msg), "(%d of %d found)", found, num_);
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
};

int main(int argc, char** argv) 
{
  std::string default_db_path;

  for (int i = 1; i < argc; i++) {
    double d;
    int n;
    char junk;
    string str = argv[i];

    if (str.find("--benchmarks=")) {
      FLAGS_benchmarks = argv[i] + strlen("--benchmarks=");
    } else if (sscanf(argv[i], "--use_existing_db=%d%c", &n, &junk) == 1 && (n == 0 || n == 1)) {
      FLAGS_use_existing_db = n;
    } else if (sscanf(argv[i], "--num=%d%c", &n, &junk) == 1) {
      FLAGS_num = n;
    } else if (sscanf(argv[i], "--value_size=%d%c", &n, &junk) == 1) {
      FLAGS_value_size = n;
    } else if (sscanf(argv[i], "--cache_size=%d%c", &n, &junk) == 1) {
      FLAGS_cache_size = n;
    } else if (strncmp(argv[i], "--db=", 5) == 0) {
      FLAGS_db = argv[i] + 5;
    } else {
      fprintf(stderr, "Invalid flag '%s'\n", argv[i]);
      exit(1);
    }
  }

  if (FLAGS_db == NULL) {
      default_db_path = "dbbench";
      FLAGS_db = default_db_path.c_str();
  }

  Benchmark benchmark;
  benchmark.Run();
  return 0;
}

# PageDB

------

`PageDB` is a key-value storage library and it uses [Extendible_hashing](http://en.wikipedia.org/wiki/Extendible_hashing) as internal structure. Also, I have implemented LRU-Cache and FIFO-Cache in its external Cache-System. Anyone, who has interest in it, can add self-customed structure or Cache-Schedule Algorithm to enhance its ability. That is the reason I renamed it as `PageDB`.

Our work is mainly inspired by [APUE](www.apuebook.com), [GDBM](www.gnu.org/s/gdbm/), [leveldb](https://code.google.com/p/leveldb/) and [yodb](https://github.com/kedebug/yodb). Thanks again for open-source group.

## Features

> * Keys and values are arbitrary byte arrays.
> * Byte arrays are represented by `Slice`
> * The basic operations are Put(key,value), Get(key), Remove(key), Put(batch).
> * Custom-option Ability, like turn on/off log. 
> * Exposed `DBImpl`(database implementation) interface and Cache interface.
> * Little RAM compared with is `leveldb` required.
> * Small space for Index compared with `leveldb`

## Limitations

> * This is not a SQL database. It does not have a relational data model, it does not support SQL queries, and it has no support for indexes.
> * Only a single process can access a particular database at a time. However, multiple-threading can be accepted. For the detail, read `db_parallelbatch.cpp` in detail.
> * Only localhost db is supported.
> * No compression tool is provided in the db. 
> * only random write and random read are supported.

## Performance
Here is a performance report (with explanations) from the run of `db_benchmark.cpp` and `db_benchmark.h`. The results are somewhat noisy, but should be enough to get a ballpark performance estimate.

### 1. Setup
We tried our database with a million entries. Each entry has a 16 byte key, and a 100 byte value. Too get maximum performance, we turned off log and cache(As the space of the key is far larger than we put, the cache has little performance balance.)

```
PageDB:   version 1.1
Date:       Tue Jun 17 12:58:54 2014
CPU:        4 * Intel(R) Core(TM) i5-3317U CPU @ 1.70GHz
CPUCache:   3072 KB
Keys:       16 bytes each
Values:     100 bytes each
Entries:    1000000
RawSize:    116 MB (estimated)
FileSize:   143 MB (estimated)
WARNING: Log   is disabled, for better performance.
WARNING: Cache is disabled, for better performance.
```

### 2. Write performance
The "fill" benchmarks create a brand new database, in random order. The "fillsync" benchmark flushes data from the operating system to the disk after every operation; the other write operations leave the data sitting in the operating system buffer cache for a while.

```
fillsync        :       46.0 micros/op;    2.39 MB/s     
fillbatch       :        7.6 micros/op;   14.43 MB/s 
fillthreadbatch :        7.2 micros/op;   15.27 MB/s
```
Each "op" above corresponds to a read/write of a single key/value pair. . I.e., a sync write benchmark goes at approximately 20,000 writes per second.

### 3. Read performance
We list the performance of a random lookup. Note that the database created by the benchmark is quite small. Therefore the report characterizes the performance of `leveldb` when the working set fits in memory. Write performance will be mostly unaffected by whether or not the working set fits in memory
```
readrandom      :       3.0 micros/op;   36.00 MB/s  
```

### 4. Compression Ration
PageDB provide compact feature to compression raw database. However, it only can compress fillsync, which is the most application situation. That can be called time-memory trade-off technology.
```
            Before Compression  |  After Compression
Raw Size:   230 MB                   131 MB
```

## Usage
I test our `CustomdDB` it on Ubuntu 13.10, so if you have any problem, please feel free to send email to me.

### 1. Compile and Installation
`PageDB` use GNU Make to handle compilation, you can find detail information from [this url](www.gnu.com/Make/).
```
$ make
$ make tests
$ make db_tests
$ sudo make install
```

### 2. Detail Usage

#### 2.1 `Slice`
`Slice` represents the immutable buffer and it is used as the basic representation of key/value. In a nutshell, `Slice` is the archive of byte arrays.

```
Slice slice("hello world");
Slice slice("hello world", 11);
...
```

#### 2.1 Open & Close `PageDB`

```c++
   Option option;
   PageDB db = new PageDB;
   db -> open(option);  
   db -> close();
   delete db;
```

#### 2.2 Dump Into Stream
Stream can either be terminal or file stream

```c++
   Option option;
   PageDB db = new PageDB;
   db -> open(option);  
   db -> dump(std::cout);
   db -> close();
   delete db;
```
#### 2.3 Put Elements
Put is equal to replace. In a nutshell, put will replace the value existed in the db or put a new value into db.

```c++
   Slice key("slice"); 
   Slice value("123213");
   db -> put(key, value); 
   db -> put("123213","iipppp");
```

#### 2.4 Get Elements

```c++
   Slice key("slice"); 
   Slice value = db -> get(key); 
   assert(value.size() != 0); //Not empty.
```

#### 2.5 Remove Elements

```c++
   Slice key("slice");
   bool successful = db -> remove(key);
```

#### 2.6 Batch Write

```c++
   WriteBatch batch(3);
   batch.put(k1, k2);
   batch.put(k2, k3);
   batch.put(k3, k1);
   db -> put(&batch);
```

#### 2.7 Multiple_Threading Write
`PageDB` supports multiple-threading and it support write/get in different threads at the same time. To find the detail information, seeking the example in `tests\db_parallelbatch.cpp`.

### 3. Other Feature
#### 3.1 Unit Test Module

```c++
    #include "TestUtils.h"
    using namespace utils;
    
    class A { };
    Test(A, Test1) { ASSERT_EQ(*,*)}
    Test(A, Test2) {}
    
    RunAllTests();
```

#### 3.2 File Module

```c++
    #include "FileModule.h"
    using namespace utils;
    
    FileModule::Size(string filename) // Return the size of file
    FileModule::Exist(string filename) // Exist or not
    
    RandomFile  file(string filename) // Random File
    file.Read(buff, offset, size)
    file.Write(buff, offset, size)
```

![PageDB UML Graph](https://raw.githubusercontent.com/mathewes/blog-dot-file/master/PageDB.png)

## Todo List
> * Parallel Map(like concurrentmap)
> * Backend Job
> * Support parallel/put/get/../
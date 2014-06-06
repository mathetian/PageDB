#ifndef _FILE_MODULE_H
#define _FILE_MODULE_H

#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#include <assert.h>

#include <sys/stat.h>

#include "Slice.h"
using namespace customdb;

#include "Noncopyable.h"
/***
*** We Need A File Module which satisfies 
*** 1. create if not exist and won't truncate the file
***/
namespace utils
{

class FileModule
{
public:
  static bool Exist(const string &fileName)
  {
     return (access(fileName.c_str(), F_OK ) != -1) ? true : false;
  }

  static uint64_t  Size(const string &fileName)
  {
    if(Exist(fileName) == false) return 0;

    struct stat sta;

    assert(stat(fileName.c_str(), &sta) != -1);

    return sta.st_size;
  }

  static bool Remove(const string &fileName)
  {
    if(Exist(fileName) == false)
      return false;
    string cmd = "rm " + fileName;
    system(cmd.c_str());

    return true;
  }

private:
  FileModule();
};

class RandomFile : Noncopyable{
public:
  RandomFile() : openStatus_(0), fileSize_(0), fd_(-1) { }
  
  ~RandomFile()
  {
      Close();
  }

  bool Open(const string &fileName)
  {
  	openStatus_ = FileModule::Exist(fileName) == true ? 1 : 2;
  	fd_ = open(fileName.c_str(), O_RDWR | O_CREAT, 0644);
  	assert(fd_ != -1);
  	fileSize_ = lseek(fd_, 0, SEEK_END);
  	lseek(fd_, 0, SEEK_SET);

  	return true;
  }

  bool Append(const Slice &slice, size_t size)
  {
  	lseek(fd_, 0, SEEK_SET); 
  	assert(write(fd_, slice.c_str(), size) == size);
  	fileSize_ = fileSize_ + size;

    return true;
  }

  bool Read(char *str, uint64_t offset, size_t size)
  {
    assert(offset + size <= fileSize_);
    lseek(fd_, offset, SEEK_SET);
    assert(read(fd_, str, size) == size);

    return true;
  }
  
  bool Write(const Slice &result, uint64_t offset, size_t size)
  {
    assert(offset <= fileSize_);
    lseek(fd_, offset, SEEK_SET);
    assert(write(fd_, result.c_str(), size) == size);

    fileSize_ = offset + size > fileSize_ ? offset + size : fileSize_;

    return true;
  }

  bool Close()
  {
    if(openStatus_ != 0 && fd_ != -1)
      close(fd_);
    
    openStatus_ = 0; fd_ = -1;
  }

  bool Sync()
  {
    if(openStatus_ != 0)
      fsync(fd_);
  }

  uint64_t Size()
  {
    return fileSize_;
  }

  int      Status()
  {
    return openStatus_;
  }

  bool     Truncate(size_t size)
  {
    assert(size < fileSize_);
    assert(ftruncate(fd_, size) ==0);

    fileSize_ = 0;
    return true;
  }

private:
  string fileName_;
  int    openStatus_;
  int    fd_;
  uint64_t fileSize_;

  RandomFile(const RandomFile&);
  void operator=(const RandomFile&);
};


}


#endif
// Copyright (c) 2014 The CustomDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _FILE_MODULE_H
#define _FILE_MODULE_H

#include "CommonHeader.h"

#include "Slice.h"
#include "Noncopyable.h"

/**
** FileModule.h is used to simplify operation for File
**/

namespace utils
{

/**
** FileModule provides some common helper function for File operate.
**/
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

/**
** RandomFile is for RandomFile
**
** In general situation, there are many types of files.
** In our implementation, we only need RandomFile
**/
class RandomFile : Noncopyable
{
public:
    RandomFile() : openStatus_(0), fileSize_(0), fd_(-1) { }

    ~RandomFile()
    {
        Close();
    }

    /**
    ** Open a file. If this file doesn't exist, creating it
    **/
    bool Open(const string &fileName)
    {
        openStatus_ = FileModule::Exist(fileName) == true ? 1 : 2;
        fd_ = open(fileName.c_str(), O_RDWR | O_CREAT, 0644);
        assert(fd_ != -1);
        fileSize_ = lseek(fd_, 0, SEEK_END);
        lseek(fd_, 0, SEEK_SET);

        return true;
    }

    bool Read(char *str, uint64_t offset, size_t size)
    {
        assert(offset + size <= fileSize_);
        lseek(fd_, offset, SEEK_SET);
        assert(read(fd_, str, size) == size);

        return true;
    }

    /**
    ** To simply code, we support slice and char*
    **
    ** Append is to append content in end of the file
    **/
    bool Append(const Slice &slice)
    {
        return Append(slice.c_str(), slice.size());
    }

    bool Append(const char *str, size_t size)
    {
        lseek(fd_, 0, SEEK_END);
        assert(write(fd_, str, size) == size);
        fileSize_ = fileSize_ + size;

        return true;
    }

    bool Write(const Slice &slice, uint64_t offset)
    {
        return Write(slice.c_str(), offset, slice.size());
    }

    bool Write(const char *result, uint64_t offset, size_t size)
    {
        assert(offset <= fileSize_);
        lseek(fd_, offset, SEEK_SET);
        assert(write(fd_, result, size) == size);

        fileSize_ = offset + size > fileSize_ ? offset + size : fileSize_;

        return true;
    }

    /**
    ** Truncate file into speical length
    **/
    bool     Truncate(size_t size)
    {
        assert(size < fileSize_);
        assert(ftruncate(fd_, size) ==0);

        fileSize_ = 0;
        return true;
    }

    bool Close()
    {
        if(openStatus_ != 0 && fd_ != -1)
            close(fd_);

        openStatus_ = 0;
        fd_ = -1;
    }

    bool Sync()
    {
        if(openStatus_ != 0)
            fsync(fd_);
    }

    uint64_t size()
    {
        return fileSize_;
    }

    int      Status()
    {
        return openStatus_;
    }

private:
    string fileName_;
    uint64_t fileSize_;
    int    fd_;
    /**
    ** 0: without open operation
    ** 1: Exist 2: Not exist
    **/
    int    openStatus_;

};

}


#endif
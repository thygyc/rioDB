#ifndef FILE_HANDLE_HPP
#define FILE_HANDLE_HPP

#include "core.hpp"
#include "logging.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
封装对文件的操作
*/

const int INVALID_HANDLE = -1;  // 无效的文件

using handleType = int;
using offsetType = off_t;  // 8字节大小， long long int 类型

class FileHandleOp
{
   public:
    explicit FileHandleOp(): file_handle_(INVALID_HANDLE){} 
    ~FileHandleOp();
    int Open(const char* path, const size_t size);
    int Read(void* const buf, const size_t size, offsetType offset);
    int Read(void* const buf, const size_t size);
    int Write(const void* buf, const size_t size);

    const char* FindFileName(const char* path);
    offsetType getCurOffset() const{
        return lseek(file_handle_, 0, SEEK_CUR);
    }
    void seekToOffSet(offsetType offset);
    void seekToEnd(){ lseek(file_handle_, 0, SEEK_END); }
    handleType handle() const { return file_handle_; }
    bool isValid() const { return file_handle_ != INVALID_HANDLE; }  // 文件描述符是否有效
   
    void Close();
    bool Ifmmap() { return if_mmaped; }
    void SetIfmmaped(bool flag) { if_mmaped = flag; }
    size_t FileOccupation() { return occupation_; }
    std::string GetFileName(){ return file_name_; }

   private:
    handleType file_handle_;  // 打开文件的文件描述符
    std::string file_name_;  // 打开的文件名
    size_t occupation_ = 0;  // 返回文件中已占用的空间大小
    bool if_mmaped = false;  // 该文件是否已经被 mmap。
    const int kFileFlag = O_RDWR | O_CREAT;  // open 文件时的权限控制
    const int kFileMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;  // 自己读写权限，组读权限，其他读权限
};

#endif


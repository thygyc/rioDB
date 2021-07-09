#ifndef RIOCCTAN_DBFILE_HPP
#define RIOCCTAN_DBFILE_HPP

#include "core.hpp"
#include "fileHandle.hpp"
class DBfile
{
   public:
    explicit DBfile(size_t dbMaxFileSize  = 1024 * 1024 * 8): dbMaxFileSize_(dbMaxFileSize){}
    ~DBfile(){}
    int Open(const char* path);
    int Read(void* const buf, const size_t size, offsetType offset);
    int Write(const void* buf, const size_t size);
    int Map(void** address, size_t size);
    int Read(void* const buf, const size_t size);

    FileHandleOp GetFileHandle() { return file_handle_; }

    size_t GetFileFreeSpace();  // 获取剩余文件空间
    size_t GetFileOccupation();  // 获取已占用的文件空间

    void SeekToStart(){ return file_handle_.seekToOffSet(0); }
    std::string GetFileName(){ return file_name_; }
    
    int Close();

   private:
    bool if_Maped = false;
    FileHandleOp file_handle_;
    std::string file_name_;
    size_t dbMaxFileSize_;
    std::mutex mtx_;  // 控制文件的读写。因为涉及到对文件的读写指针的定位操作，所以这里无法用读写锁
};

#endif
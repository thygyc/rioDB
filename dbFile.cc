#include "dbFile.hpp"

int DBfile::Open(const char* path)
{
    if(file_handle_.Open(path, dbMaxFileSize_) == -1)
    {
        DB_LOG(error, "Fail to open DBfile");
        return -1;
    }
    file_name_ = file_handle_.GetFileName();
    
    return OK;
}

// @brief: 从文件距离开头 offset 处读取 size 个字节，返回读取的字节数，读取失败返回-1。
int DBfile::Read(void* const buf, const size_t size, offsetType offset)
{
    std::lock_guard<std::mutex> lock(mtx_);
    int ret = file_handle_.Read(buf, size, offset);
    return ret;
}

//@brief：从文件当前读写指针处读 size 个字节
int DBfile::Read(void* const buf, const size_t size)
{
    std::lock_guard<std::mutex> lock(mtx_);
    int ret = file_handle_.Read(buf, size);
    return ret;
}

// @brief: 写入buf的size个字节到文件，写入成功返回 Ok,写入失败返回-1
int DBfile::Write(void const* buf, const size_t size)
{
    std::lock_guard<std::mutex> lock(mtx_);
    int ret = file_handle_.Write(buf, size);
    return ret;
}

// @brief: 用于获取文件剩余大小。这里也要上锁
size_t DBfile::GetFileFreeSpace()
{
    std::lock_guard<std::mutex> lock(mtx_);
    size_t free = dbMaxFileSize_ - file_handle_.FileOccupation();
    return free;
}

size_t DBfile::GetFileOccupation()
{
    std::lock_guard<std::mutex> lock(mtx_);
    return file_handle_.FileOccupation();
}

// mmap 已打开的文件
int DBfile::Map(void** address, size_t size)
{   
    if(file_handle_.Ifmmap()){
        return OK;
    }
    else
    {
        auto result = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, file_handle_.handle(), 0);
        if(result == MAP_FAILED){
            DB_LOG(error, "mmap failed");
            return -1;
        }
        file_handle_.SetIfmmaped(true);
        *address = result;
        return OK;
    }
}
int DBfile::Close()
{
    file_handle_.Close();
    return OK;
}
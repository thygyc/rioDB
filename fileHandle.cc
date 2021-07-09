#include "fileHandle.hpp"

FileHandleOp::~FileHandleOp()
{
    close(file_handle_);
    file_handle_ = INVALID_HANDLE;
}

// 打开文件并扩展文件大小至 size， 同时调整文件读写指针到文件开头
int FileHandleOp::Open(const char* path, const size_t size)
{
    if(isValid())
    {
        DB_LOG(info, "new file has been created");
        return OK;
    }
    do
    {
        file_handle_ = open(path, kFileFlag, 0777);
    }while(file_handle_ == INVALID_HANDLE && errno == EINTR);
    
    file_name_ = std::string(FindFileName(path));
    // 扩展文件大小至 size+5，留有一定富余量；调整文件读写指针的位置到文件开头
    int ret = ftruncate(file_handle_, size + 5);
    if(ret == -1){
        DB_LOG(error, "fail to truncate file ");
        return -1;
    }
    lseek(file_handle_, 0, SEEK_SET);
    occupation_ = 0;

    return OK;
}

// @brief: 从文件开头 offset 处读取 file_handel_指向的文件 size 个字节大小的内容，返回读取的字节数，写入失败或只写入部分返回 -1
int FileHandleOp::Read(void* const buf, const size_t size, offsetType offset)
{
    int bytesRead = 0, length = 0;
    lseek(file_handle_, offset, SEEK_SET);

    if(isValid())
    {
        do
        {
            bytesRead = read(file_handle_, buf, size);
            if(bytesRead > 0)
            {
                length += bytesRead;
            }
        } while ((bytesRead == -1 && errno == EINTR) || (length < size));

        if(bytesRead == -1 && errno != EINTR)
        {
            DB_LOG(error, "read fail");
            return -1;
        }
        return length;
    }
    else
    {
        DB_LOG(error, "Faile to Read file");
        return -1;
    }
}

// 从以打开文件的文件指针处开始读，封装一下保证读取完全
int FileHandleOp::Read(void* const buf, const size_t size)
{
    int bytesRead = 0, length = 0;
    if(isValid())
    {
        do
        {
            bytesRead = read(file_handle_, buf, size);
            if(bytesRead > 0)
            {
                length += bytesRead;
            }
        } while ((bytesRead == -1 && errno == EINTR) || (length < size));

        if(bytesRead == -1 && errno != EINTR)
        {
            DB_LOG(error, "read fail");
            return -1;
        }
    }
    else
    {
        DB_LOG(error, "Faile to Read file");
        return -1;
    }
}

// @brief: 写入buf 的size个字节到文件，写入成功返回 Ok,写入失败返回-1。
//         写之前会根据 occupation 变量调整读写指针到最后一个 record 的末尾处
// @note: 注意需要配合 occupation 变量
int FileHandleOp::Write(const void* buf, const size_t size)
{
    auto rc = OK;
    size_t current_size = 0;
    lseek(file_handle_, occupation_, SEEK_SET);
    // printf("%s", (char*)buf);
    if(isValid())
    {
        // 当写入的数据数量小于 size 的时候，往文件里接着写
        do
        {
            rc = write(file_handle_, &((char*)buf)[current_size], size - current_size);
            if(rc >= 0){
                current_size += rc;
            }
        }while((-1 == rc && EINTR == errno) || (-1 != rc && current_size < size));

        // printf("%s\n", strerror(errno));
        if(rc == -1)
        {
            DB_LOG(error, "Fail to write file");
        }
        // 更新已占用空间
        occupation_ += size;
    }
    else
    {
        DB_LOG(error, "Fail to read file, file_handle is invalid");
        return -1;
    }
    return rc;
}

void FileHandleOp::seekToOffSet(offsetType offset)
{
    if((offsetType)(-1) != offset)
    {
        lseek(file_handle_, offset, SEEK_SET);
    }
}

// 关闭文件，如果文件已关闭则不做操作
void FileHandleOp::Close()
{
    if(isValid())
    {
        close(file_handle_);
        file_handle_ = INVALID_HANDLE;
        if_mmaped = false;
    }
}
// 从文件路径中获得文件名
const char* FileHandleOp::FindFileName(const char* path)
{
    int sep = '/';
    if(path == NULL){
        return NULL;
    }
    const char* file_name = strrchr(path, sep);
    return (file_name == NULL) ? NULL : (file_name + 1);
}


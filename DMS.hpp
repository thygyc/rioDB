#ifndef RIOCCTAN_DMS_HPP
#define RIOCCTAN_DMS_HPP

#include "dbFile.hpp"
#include <map>
#include <stdio.h>
#include <dirent.h>

const char RecordeButtomCatcher = 0x20;  // record末尾的标识符。ascii码转换后是空格
const char FileEnd = 0x20;  // db文件的末尾标识符。
struct ElementHash
{
    size_t version;  // 版本号
    size_t value_sz;  // value 的大小。这包括了最末尾的‘\0’
    size_t record_pos;  // record 在磁盘文件中的偏移
    std::string file_id;  // record 所在磁盘文件的文件名
};
/*
record 结构：
-----------------
    RecordHeader
    key
    value
    0x20
-----------------
key 以 ‘\0’结尾
value以'\0'结尾
0x20是record结束符
*/

/*
dbfile 结构
---------------
record
record
...
record
0x20
-----------
0x20 是文件末尾结束符标志
*/

struct RecordHeader
{
    char RecordTopCatcher[4];  // record 开头标识符 ，为“PAGH”
    size_t key_sz;  // key 大小
    size_t value_sz;  // value 大小
    uint8_t isvalid;  // 标志位，0 表示记录无效，1 表示记录有效
    size_t version;  // 版本号
};

class DMScore
{
   public:
    explicit DMScore(size_t);
    ~DMScore();
    int Query(std::string key, std::string& rvalue);  // 查询操作
    int Insert(std::string key, std::string value, bool ifdelete = false);  // 插入操作
    int Delete(std::string key);  // 删除操作
    int Merge();  // 合并整理旧文件操作
    int init();  // 初始化操作
    int load_data();  // 从"./DB"目录中加载数据文件
    bool CheckTopCatcher(RecordHeader* record_header);  // 判断一个 record 的文件头守卫
    const char* GetNewFileName();  // 根据 tag_ 返回新的文件名, 带目录名，“./DB/dbfile_xxx.db”    
   private:
    std::multimap<std::string, ElementHash>* KEmap_cur_;  // 活跃文件的哈希表
    
    std::multimap<std::string, ElementHash>* KEmap_old_;  // 旧文件的哈希表
    
    std::map<std::string, DBfile* >* Filemap_old_;  //  旧文件的名字到对应句柄的哈希表
    
    DBfile* DbFileActive_;  // 当前活跃文件的句柄

    size_t version_ = 0; // record 版本号
    size_t tag_ = 0; // 文件的版本号

    boost::shared_mutex mtx_KEcurAc_;  // KEmap_cur 和 DbFileActive_ 的读写锁
    boost::shared_mutex mtx_KEoldFile_;  // Filemap_old_ 和 KEmap_old_ 的读写锁
    boost::shared_mutex mtx_InsertNoFree_;  // 在剩余空间不充足时的插入操作，与其他操作互斥，用读写锁互斥
    size_t dbMaxFileSize_;  // db 文件的最大大小
};

#endif


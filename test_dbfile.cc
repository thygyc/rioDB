#include <cstdio>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <map>
#include <iostream>
using namespace std;

struct RecordHeader
{
    char RecordTopCatcher[4];  // record 开头标识符 ，为“PAGH”
    size_t key_sz;  // key 大小
    size_t value_sz;  // value 大小
    uint8_t isvalid;  // 标志位，0 表示记录无效，1 表示记录有效
    size_t version;  // 版本号
};

int main()
{
    int fd = open("./DB/dbfile_1.db", O_RDWR);
    
    RecordHeader head;
    int ret = read(fd, &head, sizeof(head));
    char key[20];
    char value[20];
    read(fd, key, head.key_sz);
    read(fd, value, head.value_sz + 1);

    return 0;

}
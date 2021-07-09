#include "dbFile.hpp"
#include "core.hpp"
int main()
{
    DBfile dbfile(20);
    dbfile.Open("./DB/dbfile_0.db");
    
    char buf[20]={"12345"};
    dbfile.Write(buf, 5);
    char buf2[20];
    dbfile.Read(buf2, 2, 2);
    dbfile.Write(buf,5);
    size_t freespace = dbfile.GetFileFreeSpace();
    printf("%ld\n", freespace);
    printf("%s\n", buf2);
    return 0;
}
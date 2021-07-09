#include "DMS.hpp"

DMScore::DMScore(size_t dbMaxFileSize) : dbMaxFileSize_(dbMaxFileSize)
{
    int ret = init();
    if(ret != OK)
    {
        DB_LOG(error, "DMScore init fail");
    }
}

DMScore::~DMScore()
{
    // 关闭时给数据文件加上文件结束符
    DbFileActive_->Write(&FileEnd, 1);

    delete KEmap_cur_;
    delete KEmap_old_;
    delete DbFileActive_;
    
    // 因为 Filemap_old_的 value 是指针，所以要逐个 delete
    for(auto iter = Filemap_old_->begin(); iter != Filemap_old_->end(); ++iter)
    {
        delete iter->second;
    }
    delete Filemap_old_;
}

int DMScore::init()
{
    auto rc = load_data();
    if(rc != OK){
        return -1;
    }
    KEmap_cur_ = new std::multimap<std::string, ElementHash>();
    DbFileActive_ = new DBfile(dbMaxFileSize_);

    const char* name = GetNewFileName();
    DbFileActive_->Open(name);  // db文件序号从 tag+1 开始
    delete name;

    return OK;
}

const char* DMScore::GetNewFileName()
{
    tag_ += 1; 
    char *name = (char*)malloc(256);
    snprintf(name, 256, "./DB/dbfile_%lu.db", tag_);
    return name;
}

int DMScore::Query(std::string key, std::string &rvalue)
{
    boost::shared_lock<boost::shared_mutex> readlock(mtx_InsertNoFree_);
    
    {   
        //  先从KEmap_cur 和 DbfileActive 开始查询 key
        boost::shared_lock<boost::shared_mutex> rlock(mtx_KEcurAc_);

        // for(auto iter = KEmap_cur_->begin(); iter != KEmap_cur_->end(); ++iter)
        // {
        //     std::cout << "KEmap_cur_:" << std::endl;
        //     std::cout << iter->first << std::endl;
        // }

        if(!KEmap_cur_->empty() && DbFileActive_ != nullptr)
        {
            auto pr = KEmap_cur_->equal_range(key);
            
            if(pr.first != pr.second)
            {
                // 先找到version最大的
                auto iter = pr.first;
                auto max_version_iter = iter;
                for(; iter != pr.second; ++iter)
                {
                    max_version_iter = max_version_iter->second.version > iter->second.version ? max_version_iter : iter;
                }
                iter = max_version_iter;
                // 从文件读取 该 version 最大的 record
                RecordHeader header;
                DbFileActive_->Read((void*)(&header), sizeof(header), iter->second.record_pos);
                if(!header.isvalid)
                {
                    // 该记录无效
                    return -1;
                }
                else
                {
                    // 读取 value
                    char buf[iter->second.value_sz];
                    DbFileActive_->Read((void*)buf, iter->second.value_sz, iter->second.record_pos + sizeof(RecordHeader) + header.key_sz);
                    rvalue = std::string(buf);
                    return OK;
                }
            }
        }
    }

    {
        // for(auto iter = KEmap_old_->begin(); iter != KEmap_old_->end(); ++iter)
        // {
        //     std::cout << "KEmap_old_:" << std::endl;
        //     std::cout << iter->first << std::endl;
        // }

        // 从 KEmap_old_ 和 Filemap_old_中查询
        boost::shared_lock<boost::shared_mutex> rlock(mtx_KEoldFile_);
        if(!KEmap_old_->empty() && Filemap_old_ != nullptr)
        {
            auto pr = KEmap_old_->equal_range(key);
            if(pr.first != pr.second)
            {
                auto iter = pr.first;
                auto max_version_iter = iter;
                for(; iter != pr.second; ++iter)
                {
                    max_version_iter = max_version_iter->second.version > iter->second.version ? max_version_iter : iter;
                }
                iter = max_version_iter;
                // 从 Filemap_old_中查找对应的文件句柄
                auto dbfile = Filemap_old_->find(iter->second.file_id);
                if (dbfile != std::end(*Filemap_old_))
                {
                    RecordHeader header;
                    dbfile->second->Read((void*)(&header), sizeof(header), iter->second.record_pos);
                    if(!header.isvalid)
                    {
                        // 该记录无效
                        return -1;
                    }
                    else
                    {
                        // 读取 value
                        char buf[iter->second.value_sz];
                        dbfile->second->Read((void*)buf, iter->second.value_sz, iter->second.record_pos + sizeof(RecordHeader) + header.key_sz);
                        rvalue = std::string(buf);
                        return OK;
                    }
                }
                else
                {
                    // 没有对应文件句柄
                    return -1;
                }
            }
            else
            {
                // 没有对应的 key
                return -1;
            }
        }
    }
    return -1;
}

int DMScore::Insert(std::string key, std::string value, bool ifdelete)
{
    {
        boost::shared_lock<boost::shared_mutex> readlock(mtx_InsertNoFree_);
        // 先计算当前活跃文件剩余空间的大小
        {
            // 操作会更改信息，加写锁
            boost::unique_lock<boost::shared_mutex> writelock(mtx_KEcurAc_);
            size_t freespace = DbFileActive_->GetFileFreeSpace();
            // printf("%lu\n", freespace);
            size_t record_size = sizeof(RecordHeader) + key.size() + 1 + value.size() + 1 + 1;
            if(freespace >= record_size) 
            {
                // 设置 RecordHeader
                RecordHeader record_header;
                if(ifdelete){
                    record_header.isvalid = 0;
                }
                else{
                    record_header.isvalid = 1;
                }
                record_header.key_sz = key.size() + 1;
                record_header.value_sz = value.size() + 1;
                version_ += 1;
                record_header.version = version_;
                record_header.RecordTopCatcher[0] = 'P';
                record_header.RecordTopCatcher[1] = 'A';
                record_header.RecordTopCatcher[2] = 'G';
                record_header.RecordTopCatcher[3] = 'H';

                // 把映射加入KEmap_cur_
                ElementHash element;
                element.file_id = DbFileActive_->GetFileName();
                element.record_pos = DbFileActive_->GetFileOccupation();
                element.value_sz = record_header.value_sz;
                element.version = version_;
                KEmap_cur_->insert(std::make_pair(key, element));

                // 将recordHeader 写入活跃文件
                DbFileActive_->Write((void *)(&record_header), sizeof(record_header));
                // 再将 key 和 value 写入文件
                const char* key_s = key.c_str();
                // value后要加入一个终止标记
                const char* value_s = value.c_str();
                DbFileActive_->Write(key_s, record_header.key_sz);
                DbFileActive_->Write(value_s, record_header.value_sz);
                DbFileActive_->Write(&RecordeButtomCatcher, 1);

                // printf("%s\n", key.c_str());
                // printf("%s\n", element.file_id.c_str());
                // printf("%lu\n", element.record_pos);
                // printf("%lu\n", element.value_sz);
                // printf("----------------------\n");
                return OK;
            }
        }
    }
    
    {
        {
            // 剩余空间不够，创建新的文件，这里要加写锁，该操作与其他所有操作互斥
            boost::unique_lock<boost::shared_mutex> writelock(mtx_InsertNoFree_);
            // 创建新的 DbFile 作为活跃文件
            DBfile* DbFileActive_new = new DBfile(dbMaxFileSize_);
            const char* newfilename = GetNewFileName();
            DbFileActive_new->Open(newfilename);
            delete newfilename;
            // DbFileActive_ 加入到 Filemap_old_
            std::string name = DbFileActive_->GetFileName();
            Filemap_old_->insert(std::make_pair(name, DbFileActive_));
            // 在该文件结尾处加上结束标记
            const char* endfile = &FileEnd;
            DbFileActive_->Write(endfile, 1);
            //把 KEmap_cur_ 并入到 KEmap_old_，清空 KEmap_cur_
            KEmap_old_->insert(KEmap_cur_->begin(), KEmap_cur_->end());
            KEmap_cur_->clear();
            // 更改活跃文件句柄
            DbFileActive_ = DbFileActive_new;
        }
        // 再进行插入
        Insert(key, value, ifdelete);
    }

    return OK;
}

int DMScore::Delete(std::string key)
{
    return Insert(key, std::string("1"), true);
}

int DMScore::Merge()
{
    boost::shared_lock<boost::shared_mutex> readlock(mtx_InsertNoFree_);
    // printf("--------------------Filemap_old_------------------\n");
    // for(auto iter = Filemap_old_->begin(); iter != Filemap_old_->end(); ++iter)
    // {
    //     std::cout << iter->first << std::endl;
    // }
    // printf("--------------------KEmap_old_--------------------\n");
    // for(auto iter = KEmap_old_->begin(); iter!= KEmap_old_->end(); ++iter)
    // {
    //     std::cout << iter->first << std::endl;
    // }

    {
        // 对 KEmap_old_和 Filemap_old_加写锁
        boost::unique_lock<boost::shared_mutex> writelock(mtx_KEoldFile_);
        // 找到所有不重复的 key
        std::set<std::string> key_set;
        for(auto iter = KEmap_old_->begin(); iter != KEmap_old_->end(); ++iter)
        {
            key_set.insert(iter->first);
        }
        // 找到对应 key 中 version 最大的，有效则插入到新文件中
        DBfile *dbfile = new DBfile(dbMaxFileSize_);  // 新创建一个文件
        const char* filename = GetNewFileName();
        dbfile->Open(filename);
        delete filename;

        std::map<std::string, DBfile* > *tempFilemap_old_ = new std::map<std::string, DBfile*>();  // 加入到临时的 Filemap_old_中
        tempFilemap_old_->insert(std::make_pair(dbfile->GetFileName(), dbfile));
        std::multimap<std::string, ElementHash>* tempKEmap_old_ = new std::multimap<std::string, ElementHash>();

        for(auto iter = key_set.begin(); iter != key_set.end(); ++iter)
        {
            // 找到 key 对应的 version 最大的记录
            auto pr = KEmap_old_->equal_range(*iter);
            auto it = pr.first;
            auto max_version_it = it;
            for(; it != pr.second; ++it)
            {
                max_version_it = max_version_it->second.version > it->second.version ? max_version_it : it;
            }
            it = max_version_it;
            // 把该文件从磁盘读取出来
            auto tempdbfilepr = Filemap_old_->find(it->second.file_id);
            if (tempdbfilepr != std::end(*Filemap_old_))
            {
                RecordHeader header;
                tempdbfilepr->second->Read((void*)(&header), sizeof(header), it->second.record_pos);
                size_t record_size = sizeof(header) + header.key_sz + header.value_sz + 1;
                while(header.isvalid)
                {   
                    // record有效，判断文件空间大小是否满足
                    if(dbfile->GetFileFreeSpace() >= record_size)
                    {
                        // 创建新的 ElementHash
                        ElementHash element;
                        element.file_id = dbfile->GetFileName();
                        element.record_pos = dbfile->GetFileOccupation();
                        element.value_sz = it->second.value_sz;
                        element.version = it->second.version;
                        // 把新的 ElementHash 和对应 key 加入到tempKEmap_old_中
                        tempKEmap_old_->insert(std::make_pair(it->first, element));
                        // 把 record 从旧文件读出来写入到新文件中
                        char buf[record_size];
                        tempdbfilepr->second->Read(buf, record_size, it->second.record_pos);
                        dbfile->Write(buf, record_size);
                        break;
                    }
                    else
                    {
                        // 空间不满足，再新建一个文件
                        dbfile->Write(&FileEnd, 1);
                        dbfile = new DBfile(dbMaxFileSize_);
                        const char* newfilename = GetNewFileName();
                        dbfile->Open(newfilename);
                        delete newfilename;
                        tempFilemap_old_->insert(std::make_pair(dbfile->GetFileName(), dbfile));
                    }
                }
            }
        }
        //加上文件结束符
        dbfile->Write(&FileEnd, 1);
        // 关闭原来的数据文件并删除
        // 因为 Filemap_old_的 value 是指针，所以要逐个 delete
        for(auto iter = Filemap_old_->begin(); iter != Filemap_old_->end(); ++iter)
        {   
            std::string dir("./DB/");
            dir += iter->first;
            delete iter->second;
            remove(dir.c_str());  // 删除对应的磁盘文件
        }
        // 删除掉原来的 Filemap_old_，再进行替换
        delete Filemap_old_;
        Filemap_old_ = tempFilemap_old_;
        delete KEmap_old_;
        KEmap_old_ = tempKEmap_old_;
    }
    return OK;
}

int DMScore::load_data()
{
    KEmap_old_ = new std::multimap<std::string, ElementHash>();
    Filemap_old_ = new std::map<std::string, DBfile*>();

    const char* dir = "./DB/";
    DIR* dp = NULL;
    dirent* sdp;
    dp = opendir(dir);

    if(dp == NULL)
    {
        if(errno == ENOENT)
        {
            mkdir(dir, 0777);
            DB_LOG(info, "DB dirctory does not exist, now has been created");
            return OK;
        }
        else
        {
            DB_LOG(error,"DB directory can not open");
            return -1;
        }
    }
    
    // 从目录中读取文件，获取最大 tag 值
    size_t tag = 0;
    char filename[256];
    std::vector<dirent*> sdp_vec;
    while((sdp = readdir(dp)) != NULL)
    {
        if(sdp->d_name[0] == '.')
        {
            continue;
        }
        sscanf(sdp->d_name, "dbfile_%lu.db", &tag);
        tag_ = tag_ > tag ? tag_ : tag;
        sdp_vec.push_back(sdp);
    }
    if(sdp_vec.empty())
    {
        return OK;
    }
    // 遍历每一个数据文件，建立 Filemap_old_和 KEmap_old_
    for(auto &iter : sdp_vec)
    {
        DBfile* dbfile = new DBfile(dbMaxFileSize_);
        snprintf(filename, 256, "%s%s", dir, iter->d_name);
        printf("%s\n", filename);
        dbfile->Open(filename);
        dbfile->SeekToStart();
        // 先检查文件开头
        RecordHeader header;
        dbfile->Read(&header, sizeof(header), 0);
        if(CheckTopCatcher(&header))
        {
            // 把文件名和句柄加入到 Filemap_old_
            Filemap_old_->insert(std::make_pair(dbfile->GetFileName(), dbfile));
            // 读取记录，并把信息加入到 KEmap_old_中
            // count 来计算 record 偏移
            ElementHash* element;
            size_t count = 0;
            while(true)
            {
                element = new ElementHash;
                element->record_pos = count;
                // 读取record头判断该 record 是否完整
                dbfile->Read(&header, sizeof(header), count);
                if(CheckTopCatcher(&header))
                {
                     // 先读取 key
                    char key[header.key_sz];
                    dbfile->Read(key, header.key_sz, count + sizeof(header));
                    // header key value 0x20
                    count += sizeof(header) + header.key_sz + header.value_sz + 1;
                    char end[2];
                    dbfile->Read(&end, 2, count-1);
                    if(end[0] != 0x20)
                    {
                        DB_LOG(error, "this record is not complete, jump next dbfile to load data");
                        break;
                    }
                    // 边界条件都符合，插入到 KEmap_old_ 中
                    element->file_id = dbfile->GetFileName();
                    element->value_sz = header.value_sz;
                    element->version = header.version;
                    KEmap_old_->insert(std::make_pair(std::string(key), *element));
                    // 检查是否到了文件有效数据末尾
                    if(end[1] == 0x20){
                        DB_LOG(info, "Read to the end of the dbfile");
                        break;
                    }
                }
                else
                {
                    DB_LOG(warning, "record format is wrong");
                    return -1;
                }
            }

        }
        else
        {
            DB_LOG(warning, "dbfile is empty or record wrong format");
            continue;
        }
    }
    return OK;
}

// @brief: 监察 record 开头守卫
bool DMScore::CheckTopCatcher(RecordHeader* header)
{
    if(header->RecordTopCatcher[0] == 'P' &&
        header->RecordTopCatcher[1] == 'A' &&
        header->RecordTopCatcher[2] == 'G' &&
        header->RecordTopCatcher[3] == 'H' ){
            return true;
        }
    else
    {
        return false;
    }
}
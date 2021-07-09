#pragma once

#include "core.hpp"
#include "json.hpp"

#include <string>
#include <cstdio>
#include <fstream>

using json = nlohmann::json;
class Config
{
   public:
    Config(){ init();};
    ~Config(){}
    void init();

   public:
    std::string ip;
    int port;
    int threadNum; 
    int dbFileMaxSize;  // db 文件的最大大小
};

void Config::init()
{
    // 读取配置文件并导入到 json 对象 j 中
    json j;
    std::ifstream jfile("config.json");
    jfile >> j;

    ip = j["ip"];
    port = j["port"];
    threadNum = j["threadNum"];
    dbFileMaxSize = j["dbFileMaxSize"];
}
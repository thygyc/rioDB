#ifndef RIOCCTAN_CORE_HPP
#define RIOCCTAN_CORE_HPP

// 包含代码需要用到的库文件
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <boost/thread/shared_mutex.hpp>
#include <cctype>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <atomic>
#include <memory>
#include <mutex>
#include "json.hpp"
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

constexpr int OK = 0;

constexpr int HasCreated = -1;
constexpr int ErrCommand = -3;
constexpr int ErrKeyNotExist = -4;  // 要查询的 key 不存在
constexpr size_t SEND_BUFFER_SIZE = 4096;  // 发送缓冲区默认大小
constexpr size_t RECV_BUFFER_SIZE = 4096;   // 接收缓冲区默认大小

#endif
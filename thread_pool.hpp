#ifndef RIOCCTAN_THREADPOOL_H
#define RIOCCTAN_THREADPOOL_H

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <thread>
#include <memory>
#include <assert.h>

class ThreadPool
{
   public:
    explicit ThreadPool(int thread_num);
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

   private:
    struct Pool{
        std::mutex mtx;  //互斥锁
        std::condition_variable cv;  // 条件变量
        bool ifclosed = false;  // 线程池关闭标志
        std::queue<std::function<void()>> workqueue;  // 请求队列
        int thread_num_ = 4;  // 线程数量
        // int queue_request_num_  // 去掉了最大请求数量，感觉这个变量没有意义
    };
    std::shared_ptr<Pool> pool_;

   public:
    template<typename T>
    void append(T&& task);  // 添加请求到队列中
    static void worker(std::shared_ptr<Pool> pool);  // 线程工作函数
};

ThreadPool::ThreadPool(int thread_num):
    pool_(std::make_shared<Pool>()){
        assert(thread_num > 0 );
        pool_->thread_num_ = thread_num;

        for(int i = 0; i < pool_->thread_num_; ++i)
        {
            std::thread th(worker, pool_);
            // 分离线程
            th.detach();
        }
    }

ThreadPool::~ThreadPool()
{
    pool_->ifclosed = true;
    pool_->cv.notify_all();
}

void ThreadPool::worker(std::shared_ptr<Pool> pool)
{
    std::unique_lock<std::mutex> lck(pool->mtx);
    while(!pool->ifclosed){
        // 使用条件变量，队列为空时线程阻塞
        pool->cv.wait(lck, [&pool]{ return !(pool->workqueue.empty()); });
        auto task = std::move(pool->workqueue.front());
        pool->workqueue.pop();
        lck.unlock();
        task();  // 执行线程函数
        lck.lock();
    }
}

template <typename T>
void ThreadPool::append(T&& task)
{
    // 尽量让锁尽快释放
    {
        std::unique_lock<std::mutex> lck(pool_->mtx);
        pool_->workqueue.emplace(std::forward<T>(task));
    }
    pool_->cv.notify_one();
}
#endif
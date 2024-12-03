//#ifndef THREAD_POOL_H
//#define THREAD_POOL_H
//
//#include <vector>
//#include <queue>
//#include <memory>
//#include <thread>
//#include <mutex>
//#include <condition_variable>
//#include <future>
//#include <functional>
//#include <stdexcept>
//
//class ThreadPool {
//public:
//    ThreadPool(size_t);
//    template<class F, class... Args>
//    auto enqueue(F&& f, Args&&... args)
//        -> std::future<typename std::result_of<F(Args...)>::type>;
//    ~ThreadPool();
//    void wait_for_all_tasks();
//private:
//    std::vector<std::thread> workers;  // 线程池中的工作线程
//    std::queue<std::function<void()>> tasks;  // 任务队列
//
//    std::mutex queue_mutex;  // 互斥锁用于保护任务队列
//    std::condition_variable condition;  // 条件变量用于任务通知
//
//    std::mutex completion_mutex;
//    std::condition_variable completion_condition;  // 条件变量用于任务完成通知
//    bool stop;  // 用于指示线程池是否停止
//    size_t active_tasks;  // 当前正在执行的任务数量
//};
//
//// 构造函数，启动指定数量的工作线程
//inline ThreadPool::ThreadPool(size_t threads)
//    : stop(false), active_tasks(0)
//{
//    for (size_t i = 0; i < threads; ++i)
//        workers.emplace_back(
//            [this]
//            {
//                for (;;)
//                {
//                    std::function<void()> task;
//
//                    {
//                        std::unique_lock<std::mutex> lock(this->queue_mutex);
//                        this->condition.wait(lock,
//                            [this] { return this->stop || !this->tasks.empty(); });
//                        if (this->stop && this->tasks.empty())
//                            return;
//                        task = std::move(this->tasks.front());
//                        this->tasks.pop();
//                        ++active_tasks;
//                    }
//
//                    task();
//
//                    {
//                        std::unique_lock<std::mutex> lock(this->queue_mutex);
//                        --active_tasks;
//                        if (this->tasks.empty() && active_tasks == 0) {
//                            completion_condition.notify_all();
//                        }
//                    }
//                }
//            }
//    );
//}
//
//// 添加新的工作任务到线程池
//template<class F, class... Args>
//auto ThreadPool::enqueue(F&& f, Args&&... args)
//-> std::future<typename std::result_of<F(Args...)>::type>
//{
//    using return_type = typename std::result_of<F(Args...)>::type;
//    auto task = std::make_shared<std::packaged_task<return_type()>>(
//        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
//    );
//
//    std::future<return_type> res = task->get_future();
//    {
//        std::unique_lock<std::mutex> lock(queue_mutex);
//
//        // 不允许在停止线程池后添加新任务
//        if (stop)
//            throw std::runtime_error("enqueue on stopped ThreadPool");
//
//        tasks.emplace([task]() { (*task)(); });
//    }
//    condition.notify_one();
//    return res;
//}
//
//// 等待所有任务完成
//inline void ThreadPool::wait_for_all_tasks() {
//    std::unique_lock<std::mutex> lock(completion_mutex);
//    completion_condition.wait(lock, [this] { return tasks.empty() && active_tasks == 0; });
//}
//
//// 析构函数，停止所有线程
//inline ThreadPool::~ThreadPool()
//{
//    {
//        std::unique_lock<std::mutex> lock(queue_mutex);
//        stop = true;
//    }
//    condition.notify_all();
//    for (std::thread& worker : workers)
//        worker.join();
//}
//
//#endif // THREAD_POOL_H


#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <type_traits> // 添加这个头文件

class ThreadPool {
public:
    ThreadPool(size_t);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>; // 使用 std::invoke_result 替换 std::result_of
    ~ThreadPool();
    void wait_for_all_tasks();
private:
    std::vector<std::thread> workers;  // 线程池中的工作线程
    std::queue<std::function<void()>> tasks;  // 任务队列

    std::mutex queue_mutex;  // 互斥锁用于保护任务队列
    std::condition_variable condition;  // 条件变量用于任务通知

    std::mutex completion_mutex;
    std::condition_variable completion_condition;  // 条件变量用于任务完成通知
    bool stop;  // 用于指示线程池是否停止
    size_t active_tasks;  // 当前正在执行的任务数量
};

// 构造函数，启动指定数量的工作线程
inline ThreadPool::ThreadPool(size_t threads)
    : stop(false), active_tasks(0)
{
    for (size_t i = 0; i < threads; ++i)
        workers.emplace_back(
            [this]
            {
                for (;;)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                        ++active_tasks;
                    }

                    task();

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        --active_tasks;
                        if (this->tasks.empty() && active_tasks == 0) {
                            completion_condition.notify_all();
                        }
                    }
                }
            }
        );
}

// 添加新的工作任务到线程池
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
-> std::future<std::invoke_result_t<F, Args...>> // 使用 std::invoke_result 替换 std::result_of
{
    using return_type = std::invoke_result_t<F, Args...>;
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // 不允许在停止线程池后添加新任务
        if (stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task]() { (*task)(); });
    }
    condition.notify_one();
    return res;
}

// 等待所有任务完成
inline void ThreadPool::wait_for_all_tasks() {
    std::unique_lock<std::mutex> lock(completion_mutex);
    completion_condition.wait(lock, [this] { return tasks.empty() && active_tasks == 0; });
}

// 析构函数，停止所有线程
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread& worker : workers)
        worker.join();
}

#endif // THREAD_POOL_H


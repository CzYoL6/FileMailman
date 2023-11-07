//
// Created by zaiyi on 2023/11/7.
//

#ifndef IMGUIOPENGL_THREADPOOL_HPP
#define IMGUIOPENGL_THREADPOOL_HPP

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) {
                            return;
                        }
                        task = std::move(tasks.front());
                        tasks.pop();
                        ++busyThreadCount;
                    }
                    task();
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        --busyThreadCount;
                        if(tasks.empty() && !busyThreadCount)
                            condition_wait_finish.notify_one();
                    }
                }
            });
        }
    }

    template <class F, class... Args>
    void enqueue(F&& f, Args&&... args) {
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (stop) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks.emplace([f, args...] { f(args...); });
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mutex);
            stop = true;
        }
        condition.notify_all();
        joinAll();
    }
    void joinAll() {
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    void WaitAll() {
        std::unique_lock<std::mutex> lock(mutex);

        condition_wait_finish.wait(lock, [this](){
            return busyThreadCount == 0 && tasks.empty();
        });
    }
    size_t getBusyThreadCount()  {
        std::unique_lock<std::mutex> lock(mutex);
        return busyThreadCount;
    }
    size_t getTasksCount(){
        std::unique_lock<std::mutex> lock(mutex);
        return tasks.size();
    }


private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    size_t busyThreadCount = 0;
    std::mutex mutex;
    std::condition_variable condition, condition_wait_finish;
    bool stop;
};

// Example usage
//void taskFunction(int id) {
//    std::cout << "Task " << id << " started." << std::endl;
//    // Perform some work
//    std::this_thread::sleep_for(std::chrono::seconds(1));
//    std::cout << "Task " << id << " completed." << std::endl;
//}

//int main() {
//    ThreadPool pool(4); // Create a thread pool with 4 threads
//
//    // Enqueue tasks
//    for (int i = 0; i < 10; ++i) {
//        pool.enqueue(taskFunction, i);
//    }
//
//    // Wait for all tasks to complete
//    pool.joinAll();
//
//    std::cout << "All tasks completed." << std::endl;
//
//    return 0;
//}

#endif //IMGUIOPENGL_THREADPOOL_HPP

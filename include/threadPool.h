#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <type_traits>


class ThreadPool {
public:
    ThreadPool(size_t n = std::thread::hardware_concurrency())
        : stop(false), activeTasks(0)
    {
        numThreads = n;
        for (size_t i = 0; i < n; ++i) {
            workers.emplace_back([this, i] {
                for (;;) {
                    std::function<void(size_t)> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this]{ return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                        ++activeTasks;
                    }
                    task(i);
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        --activeTasks;
                        finished_condition.notify_all();
                    }
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (auto& w : workers) w.join();
    }

    size_t size() const { return numThreads; }

    template<typename F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::function<void(size_t)>(std::forward<F>(f)));
        }
        condition.notify_one();
    }

    template<typename F>
    void parallel_for(size_t begin, size_t end, F&& func) {
        size_t n = workers.size();
        size_t chunk = (end - begin + n - 1) / n;

        for (size_t t = 0; t < n; t++) {
            size_t start = begin + t * chunk;
            size_t stop = std::min(end, start + chunk);
            if (start >= stop) break;

            enqueue([=, &func](size_t tid){
                for (size_t i = start; i < stop; i++)
                    func(i, tid);
            });
        }

        wait();
    }

    void wait() { // prevent segmentation faults
        std::unique_lock<std::mutex> lock(queue_mutex);
        finished_condition.wait(lock, [this]{
            return tasks.empty() && activeTasks == 0;
        });
    }

private:
    size_t numThreads;
    std::vector<std::thread> workers;
    std::queue<std::function<void(size_t)>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    std::condition_variable finished_condition;

    bool stop;
    size_t activeTasks;
};

#endif
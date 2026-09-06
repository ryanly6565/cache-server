#include <cache_server/thread_pool.hpp>
#include <stdexcept>
#include <utility>

// constructor
ThreadPool::ThreadPool(std::size_t worker_count, std::size_t max_queue_size) : max_queue_size_(max_queue_size) {
    if (worker_count == 0) {
        throw std::invalid_argument("Error: Worker count must be greater than zero.");
    }

    if (max_queue_size == 0) {
        throw std::invalid_argument("Error: Queue size must be greater than zero.");
    }

    // spawn worker threads
    workers_.reserve(worker_count);
    for (std::size_t i = 0; i < worker_count; ++i) {
        workers_.emplace_back([this]() {
            worker_loop();
        });
    }
}

// destructor
ThreadPool::~ThreadPool() {
    stop();
}

// adds a new task to the thread pool
bool ThreadPool::submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_ || tasks_.size() >= max_queue_size_) {
            return false;
        }
        tasks_.push(std::move(task));   // allow owner transfership of task
    }
    task_available_.notify_one();
    return true;
}

// commands all worker threads to stop working and wakes them
void ThreadPool::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) {
            return;
        }
        stopping_ = true;
    }

    // wake all threads and consume them (tasks in queue will finish)
    task_available_.notify_all();
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

// loop for worker threads
void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mutex_); 
            task_available_.wait(lock, [this]() {
                // lambda for checking if we should stop waiting
                return stopping_ || !tasks_.empty();
            });

            if (stopping_ && tasks_.empty()) {
                return;
            }
            
            task = std::move(tasks_.front());
            tasks_.pop();
        }

        task();
    }
}
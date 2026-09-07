#include <gtest/gtest.h>

#include <atomic>
#include <stdexcept>
#include <latch>

#include "cache_server/thread_pool.hpp"

// Test that a thread pool throws an error if given 0 workers
TEST(ThreadPoolTest, ZeroWorkersThrows) {
    EXPECT_THROW(ThreadPool pool(0, 10), std::invalid_argument);
}

// Test that a thread pool throws an error if given a 0-capcity thread pool
TEST(ThreadPoolTest, ZeroQueueCapacityThrows) {
    EXPECT_THROW(ThreadPool pool(2, 0), std::invalid_argument);
}

// Test that a task can be submitted and completed
TEST(ThreadPoolTest, SubmittedTaskExecutes) {
    ThreadPool pool{1, 1};
    std::atomic<bool> task_executed{false};

    bool accepted = pool.submit([&task_executed]() {
        task_executed = true;
    });

    ASSERT_TRUE(accepted);

    // wait for task to finish
    pool.stop();
    EXPECT_TRUE(task_executed.load());
}

// Test that multiple tasks can be submitted and completed
TEST(ThreadPoolTest, MultipleTasksExecute) {
    constexpr int task_count = 10;

    ThreadPool pool{4, task_count};
    std::atomic<int> completed_tasks{0};

    for (int i = 0; i < task_count; ++i) {
        bool accepted = pool.submit([&completed_tasks]() {
            ++completed_tasks;
        });

        ASSERT_TRUE(accepted);
    }

    pool.stop();
    EXPECT_EQ(completed_tasks.load(), task_count);
}

// Test that a full queue rejects tasks
TEST(ThreadPoolTest, TaskQueueRejectsWhenFull) {
    ThreadPool pool{1, 1};

    std::latch first_task_started{1};
    std::latch release_first_task{1};
    std::atomic<int> completed_tasks{0};

    // occupy worker with task
    ASSERT_TRUE(pool.submit([&]() {
        first_task_started.count_down();
        release_first_task.wait();
    }));

    // wait for the first task to take up the task
    first_task_started.wait();

    // fill queue with 2 tasks (1 is rejected)
    bool second_accepted = pool.submit([&]() {
        ++completed_tasks;
    });
    bool third_accepted = pool.submit([&]() {
        ++completed_tasks;
    });

    // release the worker
    release_first_task.count_down();
    pool.stop();

    EXPECT_TRUE(second_accepted);
    EXPECT_FALSE(third_accepted);
    EXPECT_EQ(completed_tasks.load(), 1);
}

// Test that the task pool rejects a submission after stopping
TEST(ThreadPoolTest, TaskQueueRejectsWhenFinished) {
    ThreadPool pool{1, 1};
    std::atomic<int> completed_tasks{0};
    
    bool accepted = pool.submit([&completed_tasks]() {++completed_tasks;});
    ASSERT_TRUE(accepted);
    pool.stop();

    accepted = pool.submit([&completed_tasks]() {++completed_tasks;});
    ASSERT_FALSE(accepted);
    
    EXPECT_EQ(completed_tasks.load(), 1);
}

// Test that all tasks finish on a stop
TEST(ThreadPoolTest, TasksAreFinishedOnStop) {
    size_t task_count = 100;

    ThreadPool pool{2, task_count};
    std::atomic<int> completed_tasks{0};

    for (int i = 0; i < task_count; ++i) {
        bool accepted = pool.submit([&completed_tasks]() {
            ++completed_tasks;
        });

        ASSERT_TRUE(accepted);
    }

    pool.stop();
    EXPECT_EQ(completed_tasks.load(), task_count);
}
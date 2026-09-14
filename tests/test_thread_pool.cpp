#include "test_framework.hpp"
#include "peregrine/thread_pool.hpp"
#include <atomic>

using namespace peregrine;

TEST_CASE(ThreadPoolSuite, ExecuteTasksConcurrently) {
    ThreadPool pool(4, 32);
    EXPECT_EQ(pool.thread_count(), static_cast<size_t>(4));

    std::atomic<int> counter(0);
    const int num_tasks = 20;

    for (int i = 0; i < num_tasks; ++i) {
        bool queued = pool.enqueue([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
        EXPECT_TRUE(queued);
    }

    pool.stop();
    EXPECT_EQ(counter.load(), num_tasks);
    EXPECT_TRUE(pool.is_stopped());
}

TEST_CASE(ThreadPoolSuite, BoundedQueueAndOverloadRejection) {
    // Thread pool with 1 worker and max queue capacity of 2
    ThreadPool pool(1, 2);

    std::atomic<bool> block_worker(true);
    std::atomic<int> executed(0);

    // Enqueue a blocking task to saturate the worker
    pool.enqueue([&block_worker, &executed]() {
        while (block_worker.load()) {
            // spin wait
        }
        executed.fetch_add(1);
    });

    // Fill queue to capacity (2 tasks)
    EXPECT_TRUE(pool.enqueue([&executed]() { executed.fetch_add(1); }));
    EXPECT_TRUE(pool.enqueue([&executed]() { executed.fetch_add(1); }));

    // The 4th task MUST be rejected because queue capacity (2) is reached
    bool rejected = pool.enqueue([&executed]() { executed.fetch_add(1); });
    EXPECT_FALSE(rejected);

    // Unblock and cleanly stop
    block_worker.store(false);
    pool.stop();
}

TEST_CASE(ThreadPoolSuite, ExceptionHandlingInWorker) {
    ThreadPool pool(2, 10);
    std::atomic<bool> second_task_ran(false);

    // Enqueue a task that throws an exception
    pool.enqueue([]() {
        throw std::runtime_error("Worker task simulated error");
    });

    // The pool should catch the error and remain operational for subsequent tasks
    pool.enqueue([&second_task_ran]() {
        second_task_ran.store(true);
    });

    pool.stop();
    EXPECT_TRUE(second_task_ran.load());
}

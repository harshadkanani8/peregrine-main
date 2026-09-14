/*
 * =========================================================================
 *     ____                              _             __ __ 
 *    /  __\___  ________  ____ ________(_)_  _____   / // / 
 *   / /_/ / -_)/ __/ -_)/ _ `/__ / / / / _ \/ -_) / // /_ 
 *  / .___/\__//_/  \__/\_, /_/  /_/_/_/_//_/\__/ /__  __/ 
 * /_/                 /___/                        /_/    
 *
 *  Peregrine++ Web Application Framework
 *  Author: Harshad M. Kanani
 *  Copyright (c) 2026 Harshad Kanani. All rights reserved.
 *  SPDX-License-Identifier: Apache-2.0
 * =========================================================================
 */

// ============================================================================
// peregrine/thread_pool.hpp
//
// High-performance, lightweight, standard C++11 Thread Pool.
// Designed for resource-constrained embedded systems (ARM Linux, MIPS, x86).
// Features bounded task queue, graceful worker termination, and load rejection.
// Part of the Peregrine C++ Web Framework.
// ============================================================================
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace peregrine {

class ThreadPool {
public:
    // Constructs the thread pool with a fixed number of workers and maximum task queue capacity.
    explicit ThreadPool(size_t thread_count = 8, size_t max_queue_size = 64)
        : stop_(false),
          active_workers_(0),
          max_queue_size_(max_queue_size == 0 ? 64 : max_queue_size) {
        
        size_t actual_threads = (thread_count == 0) ? 1 : thread_count;
        workers_.reserve(actual_threads);

        for (size_t i = 0; i < actual_threads; ++i) {
            workers_.emplace_back([this]() {
                this->worker_loop();
            });
        }
    }

    // Destructor: stops all workers and joins threads
    ~ThreadPool() {
        stop();
    }

    // Non-copyable and non-movable (RAII resource manager)
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    // Enqueues a callable task. Returns true if queued, false if queue is full or pool is stopped.
    bool enqueue(std::function<void()> task) {
        if (!task) return false;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) {
                return false;
            }
            if (max_queue_size_ > 0 && tasks_.size() >= max_queue_size_) {
                return false;  // Task queue capacity exceeded (backpressure / overload rejection)
            }
            tasks_.push(std::move(task));
        }

        cv_tasks_.notify_one();
        return true;
    }

    // Gracefully shuts down the thread pool, waiting for currently running and pending tasks to finish.
    void stop() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_) return;
            stop_ = true;
        }

        cv_tasks_.notify_all();

        for (size_t i = 0; i < workers_.size(); ++i) {
            if (workers_[i].joinable()) {
                workers_[i].join();
            }
        }
        workers_.clear();
    }

    // Introspectors
    size_t thread_count() const {
        return workers_.size();
    }

    size_t active_workers() const {
        return active_workers_.load(std::memory_order_relaxed);
    }

    size_t queue_size() {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return tasks_.size();
    }

    bool is_stopped() const {
        return stop_.load(std::memory_order_relaxed);
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable cv_tasks_;

    std::atomic<bool> stop_;
    std::atomic<size_t> active_workers_;
    size_t max_queue_size_;

    void worker_loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(this->queue_mutex_);
                this->cv_tasks_.wait(lock, [this]() {
                    return this->stop_ || !this->tasks_.empty();
                });

                if (this->stop_ && this->tasks_.empty()) {
                    return;
                }

                task = std::move(this->tasks_.front());
                this->tasks_.pop();
            }

            active_workers_.fetch_add(1, std::memory_order_relaxed);
            try {
                task();
            } catch (const std::exception& ex) {
                std::cerr << "[ThreadPool] Exception in worker task: " << ex.what() << "\n";
            } catch (...) {
                std::cerr << "[ThreadPool] Unknown exception in worker task\n";
            }
            active_workers_.fetch_sub(1, std::memory_order_relaxed);
        }
    }
};

}  // namespace peregrine

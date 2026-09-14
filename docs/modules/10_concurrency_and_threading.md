# 📦 Module 10: Concurrency Architecture & Thread Pool

Peregrine employs a high-performance, bounded C++11 **Worker Thread Pool** model. It delivers predictable latency, deterministic memory usage, and proactive overload rejection without relying on external concurrency libraries.

---

## 📋 Table of Contents

1. [Architectural Concurrency Model](#1-architectural-concurrency-model)
2. [The `peregrine::ThreadPool` Class](#2-the-peregrinethreadpool-class)
   - [Worker Lifecycle & Synchronization](#worker-lifecycle--synchronization)
   - [Bounded Task Queue & Backpressure](#bounded-task-queue--backpressure)
   - [Worker Exception Shielding](#worker-exception-shielding)
3. [Load Rejection & HTTP 503 Mitigation](#3-load-rejection--http-503-mitigation)
4. [Introspection & Metrics](#4-introspection--metrics)
5. [Thread Safety Guidelines for Developers](#5-thread-safety-guidelines-for-developers)
6. [Complete Code Examples](#6-complete-code-examples)

---

## 1. Architectural Concurrency Model

Defined in `include/peregrine/thread_pool.hpp`.

Peregrine uses a **Listener-Worker Pool Pattern**:

```text
                     [ Incoming TCP Sockets ]
                                │
                                ▼
  ┌────────────────────────────────────────────────────────┐
  │  Master Socket Thread (app.run())                      │
  │  - accept() new socket file descriptor                 │
  │  - Enqueue connection into bounded ThreadPool queue    │
  └─────────────────────────────┬──────────────────────────┘
                                │
               ┌────────────────┼────────────────┐
               │                │                │
               ▼                ▼                ▼
        [ Task Queue ]   [ Task Queue ]   [ Task Queue ]
        (Capacity: N)    (Capacity: N)    (Capacity: N)
               │                │                │
               ▼                ▼                ▼
        ┌─────────────┐  ┌─────────────┐  ┌─────────────┐
        │ Worker Th 1 │  │ Worker Th 2 │  │ Worker Th N │
        │ (Runs HTTP) │  │ (Runs HTTP) │  │ (Runs HTTP) │
        └─────────────┘  └─────────────┘  └─────────────┘
```

1. **Master Thread**: Runs inside `app.run()`. Its sole responsibility is calling `accept()`, wrapping the client file descriptor, and dispatching it to the pool. It never blocks on request parsing or business logic.
2. **Worker Pool**: A fixed number of worker threads wait on a `std::condition_variable`. When notified, an idle worker pops the connection, reads the wire bytes, runs all middlewares, dispatches the handler, writes the response, and returns to wait for the next connection.

---

## 2. The `peregrine::ThreadPool` Class

### Worker Lifecycle & Synchronization

```cpp
class ThreadPool {
public:
    explicit ThreadPool(size_t thread_count = 8, size_t max_queue_size = 64);
    ~ThreadPool(); // Calls stop() and joins all threads via RAII

    bool enqueue(std::function<void()> task);
    void stop();

    size_t thread_count() const;
    size_t active_workers() const;
    size_t queue_size();
    bool is_stopped() const;
};
```

Workers execute an internal loop:
```cpp
void worker_loop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(this->queue_mutex_);
            this->cv_tasks_.wait(lock, [this]() {
                return this->stop_ || !this->tasks_.empty();
            });

            if (this->stop_ && this->tasks_.empty()) {
                return; // Graceful exit
            }

            task = std::move(this->tasks_.front());
            this->tasks_.pop();
        }

        active_workers_.fetch_add(1, std::memory_order_relaxed);
        try {
            task();
        } catch (const std::exception& ex) {
            std::cerr << "[ThreadPool] Exception: " << ex.what() << "\n";
        } catch (...) {
            std::cerr << "[ThreadPool] Unknown exception\n";
        }
        active_workers_.fetch_sub(1, std::memory_order_relaxed);
    }
}
```

---

### Bounded Task Queue & Backpressure

Unbounded queues pose a major risk: during traffic spikes, millions of pending tasks can be buffered in RAM, eventually triggering the OS Out-Of-Memory (OOM) killer.

Peregrine enforces a strict **Bounded Queue Limit** (`max_queue_size`):
```cpp
if (max_queue_size_ > 0 && tasks_.size() >= max_queue_size_) {
    return false; // Queue is full: reject task
}
```

---

### Worker Exception Shielding

If a route handler or middleware throws an unhandled exception, the worker pool's outer exception barrier intercepts it. The worker thread **never terminates prematurely**, ensuring that thread resources remain stable across unhandled application bugs.

---

## 3. Load Rejection & HTTP 503 Mitigation

When all workers are busy and the bounded queue is full:
1. `pool.enqueue(...)` returns `false`.
2. Peregrine immediately writes an HTTP 503 response and closes the socket:
   ```text
   HTTP/1.1 503 Service Unavailable
   Content-Type: text/plain; charset=utf-8
   Content-Length: 34
   Connection: close

   503 Service Temporarily Overloaded
   ```
3. This signals upstream reverse proxies (e.g. NGINX, HAProxy, AWS ALB) to back off or redirect traffic to another instance.

---

## 4. Introspection & Metrics

Monitor thread pool saturation in real-time:

```cpp
app.get("/metrics/pool", [&pool](peregrine::Request& req) {
    peregrine::Json m = peregrine::Json::object();
    m["total_threads"] = static_cast<int>(pool.thread_count());
    m["active_workers"] = static_cast<int>(pool.active_workers());
    m["queued_tasks"] = static_cast<int>(pool.queue_size());
    return peregrine::Response::json(m);
});
```

---

## 5. Thread Safety Guidelines for Developers

Because multiple worker threads execute handlers concurrently:

### Safe (Zero Synchronization Needed):
* **`peregrine::Request` and `peregrine::Response`**: Stack-local to each thread. Safe to read and mutate freely.
* **`app.config` and registered routes**: Read-only after server startup (`app.run()`). Concurrent reads are inherently thread-safe.

### Unsafe (Requires Synchronization):
* **Global or static variables**: If multiple requests modify a shared container (e.g. `static std::map<int, User> users;`), you must protect access with a `std::mutex` or `std::lock_guard`:

```cpp
#include <mutex>

static std::mutex g_data_mutex;
static std::map<std::string, int> g_stats;

app.post("/increment", [](peregrine::Request& req) {
    std::string key = req.arg("key", "views");

    {
        std::lock_guard<std::mutex> lock(g_data_mutex);
        g_stats[key]++;
    }

    return peregrine::Response::text("Incremented");
});
```

---

## 6. Complete Code Examples

### Tuning Concurrency for High Throughput

```cpp
#include <iostream>
#include <peregrine/peregrine.hpp>

int main() {
    peregrine::App app;

    app.get("/compute", [](peregrine::Request& req) {
        // Simulate CPU-bound work
        volatile double x = 0;
        for (int i = 0; i < 500000; ++i) {
            x += i * 0.001;
        }
        return peregrine::Response::text("Computation completed");
    });

    // Configure thread concurrency tuned to host hardware
    peregrine::ServerConfig cfg;
    cfg.port = 8080;
    
    // Scale workers according to available hardware concurrency
    unsigned int cores = std::thread::hardware_concurrency();
    cfg.thread_pool_size = (cores > 0) ? (cores * 2) : 8;
    
    // Set queue depth
    cfg.max_queue_size = 256;
    
    // Slowloris timeout
    cfg.socket_timeout_seconds = 5;

    std::cout << "Launching server with " << cfg.thread_pool_size 
              << " worker threads (Queue limit: " << cfg.max_queue_size << ")...\n";

    app.run(cfg);
    return 0;
}
```

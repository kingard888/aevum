// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file thread_pool.hpp
 * @brief Defines a sophisticated, high-performance, generic thread pool for asynchronous task
 * execution.
 * @details This header file specifies the `ThreadPool` class, a fundamental component for
 * concurrent architecture. It provides a robust and efficient mechanism for managing a fixed-size
 * pool of worker threads to execute tasks asynchronously. This approach amortizes the cost of
 * thread creation and destruction over the application's lifetime, significantly improving
 * performance and resource utilization for applications with a large number of short-lived tasks.
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

/**
 * @namespace aevum::util::concurrency
 * @brief A curated collection of advanced utilities and primitives for concurrent programming and
 * high-performance multithreading.
 * @details This namespace encapsulates a suite of tools designed to abstract away the complexities
 * of thread synchronization, atomic operations, and parallel execution, providing developers with
 * robust, reusable, and efficient building blocks for concurrent software architectures.
 */
namespace aevum::util::concurrency {

/**
 * @class ThreadPool
 * @brief Manages a pool of persistent worker threads to execute asynchronous tasks efficiently.
 *
 * @details This class implements a fixed-size thread pool, a powerful pattern for concurrent task
 * execution. It maintains a central queue of tasks, which are `std::function` objects, and a
 * collection of worker threads that continuously dequeue and execute these tasks. By reusing
threads,
 * the pool avoids the high overhead of creating new threads for each task. Synchronization between
 * the producer (enqueuing thread) and consumers (worker threads) is handled gracefully using a
 * `std::mutex` and a `std::condition_variable`, which allows worker threads to sleep efficiently
 * when no tasks are available, thus conserving CPU resources. The `ThreadPool` is designed for
 * graceful shutdown; its destructor ensures that all enqueued tasks are completed before the
- * application exits.
+ * application exits. The `enqueue` method returns a `std::future`, enabling the caller to
+ * retrieve the result of the asynchronous operation at a later time.
 */
class ThreadPool {
  public:
    /**
     * @brief Constructs a `ThreadPool` with a specified number of worker threads.
     *
     * @param name_prefix A descriptive string prefix used for naming the OS-level worker threads.
     *                    This is invaluable for debugging, as it allows for easy identification of
     *                    pool threads in system monitoring tools (e.g., "Worker" yields "Worker-0",
     * "Worker-1").
     * @param threads The number of worker threads to create in the pool. If this parameter is
     * omitted, it judiciously defaults to the number of concurrent threads supported by the
     *                hardware, as determined by `std::thread::hardware_concurrency()`.
     */
    explicit ThreadPool(const std::string &name_prefix = "Worker",
                        size_t threads = std::thread::hardware_concurrency());

    /**
     * @brief Destroys the `ThreadPool`, initiating a graceful shutdown sequence.
     * @details The destructor ensures that no new tasks can be enqueued, then notifies all worker
     *          threads to wake up. The workers will process any remaining tasks in the queue before
     *          exiting their main loop. The destructor then blocks, joining each thread to
     * guarantee a clean and orderly termination.
     */
    ~ThreadPool();

    /**
     * @brief Deleted copy constructor. `ThreadPool` is non-copyable.
     * @details Copying a `ThreadPool` is disallowed to prevent complex and hazardous scenarios
     * involving the ownership and management of active threads and their associated resources.
     * @param other The `ThreadPool` to be copied (operation is deleted).
     */
    ThreadPool(const ThreadPool &) = delete;

    /**
     * @brief Deleted copy assignment operator.
     * @details Assigning one `ThreadPool` to another is prohibited to maintain clear ownership
     * semantics and prevent resource management conflicts.
     * @param other The `ThreadPool` to be assigned from (operation is deleted).
     * @return A reference to this instance (operation is deleted).
     */
    ThreadPool &operator=(const ThreadPool &) = delete;

    /**
     * @brief Deleted move constructor.
     * @details Moving a `ThreadPool` is a complex operation that is disabled to enforce a simpler,
     * safer lifecycle management where the pool is stationary.
     * @param other The `ThreadPool` to be moved (operation is deleted).
     */
    ThreadPool(ThreadPool &&) = delete;

    /**
     * @brief Deleted move assignment operator.
     * @details Move assignment is disallowed for the same reasons as move construction, ensuring
     * predictable behavior and resource ownership.
     * @param other The `ThreadPool` to be move-assigned from (operation is deleted).
     * @return A reference to this instance (operation is deleted).
     */
    ThreadPool &operator=(ThreadPool &&) = delete;

    /**
     * @brief Enqueues a callable task for asynchronous execution by a worker thread.
     *
     * @details This variadic template method accepts a callable entity (e.g., a function, lambda,
     * or functor) and its arguments. It perfectly forwards these arguments and packages the call
     * into a `std::packaged_task`. The task is then type-erased into a `std::function<void()>`
     * and placed onto the thread-safe queue. The method returns a `std::future` which will
     * eventually hold the result of the function's execution, allowing the caller to synchronize
     * with the task's completion and retrieve its return value.
     *
     * @tparam F The type of the callable object.
     * @tparam Args The types of the arguments to be passed to the callable.
     * @param f The callable object to execute asynchronously.
     * @param args The arguments to be forwarded to the callable object.
     * @return A `std::future<return_type>` where `return_type` is the result type of the callable.
     *         This future can be used to wait for the task and get its result.
     * @throws `std::runtime_error` if `enqueue` is called after the thread pool has been signaled
     *         to stop, preventing new work from being added during shutdown.
     */
    template <class F, class... Args>
    [[nodiscard]] auto enqueue(F &&f, Args &&...args)
        -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
        // `std::invoke_result_t` (C++17) is used to elegantly deduce the return type of the
        // callable `F` when invoked with the given `Args...`. `std::decay_t` removes cv-qualifiers
        // and references.
        using return_type = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

        // A `std::packaged_task` is created to bundle the callable and a `std::promise`, which will
        // store the result or exception. A shared pointer is used to manage its lifetime across
        // threads.
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        // The `std::future` is retrieved from the packaged_task before it is moved into the queue.
        // This future is the handle that the caller will use to interact with the asynchronous
        // result.
        std::future<return_type> res = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            // A critical check to ensure that no new tasks are accepted once the pool has initiated
            // its shutdown sequence. This prevents race conditions during destruction.
            if (stop_.load(std::memory_order_relaxed)) {
                throw std::runtime_error("Cannot enqueue on a stopped ThreadPool");
            }

            // The packaged_task, captured by a lambda, is type-erased into a
            // `std::function<void()>` and enqueued. This allows the queue to hold heterogeneous
            // tasks.
            tasks_.emplace([task]() {
                (*task)();
            });
        }

        // A single waiting worker thread is notified that a new task is available. `notify_one` is
        // more efficient than `notify_all` when only one task is added.
        condition_.notify_one();
        return res;
    }

  private:
    /**
     * @var workers_
     * @brief A vector of `std::thread` objects representing the pool of worker threads.
     */
    std::vector<std::thread> workers_;

    /**
     * @var tasks_
     * @brief A queue of `std::function<void()>` objects. This serves as the task queue where
     * producers place work to be consumed by the worker threads.
     */
    std::queue<std::function<void()>> tasks_;

    /**
     * @var queue_mutex_
     * @brief A `std::mutex` to ensure exclusive, thread-safe access to the shared `tasks_` queue.
     * It must be locked before any modification or inspection of the queue.
     */
    mutable std::mutex queue_mutex_;

    /**
     * @var condition_
     * @brief A `std::condition_variable` used for efficient synchronization between producer and
     * consumer threads. Workers wait on this variable when the task queue is empty.
     */
    std::condition_variable condition_;

    /**
     * @var stop_
     * @brief An atomic boolean flag used to signal the termination state to all worker threads.
     * When set to `true`, workers will cease fetching new tasks and begin to exit.
     */
    std::atomic<bool> stop_;
};

}  // namespace aevum::util::concurrency

// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file thread_pool.cpp
 * @brief Provides the concrete implementation of the high-performance `ThreadPool` class.
 * @details This source file contains the definitions for the `ThreadPool` constructor and
 * destructor, detailing the logic for thread creation, the worker's main execution loop, and the
 * graceful shutdown procedure. The implementation focuses on robust synchronization and efficient
 * task processing.
 */
#include "aevum/util/concurrency/thread_pool.hpp"

#include <string>

#include "aevum/util/concurrency/thread_name.hpp"

namespace aevum::util::concurrency {

/**
 * @brief Constructs the `ThreadPool` and initializes its worker threads.
 *
 * @details This constructor is responsible for bringing the thread pool to life. It reserves space
 * for the worker threads to prevent vector reallocations and then proceeds to spawn the requested
 * number of threads. Each thread is given a unique, descriptive name derived from the provided
 * `name_prefix` to aid in diagnostics and debugging.
 *
 * The core of each worker thread is an infinite loop that forms the consumer part of a
 * producer-consumer pattern. Inside this loop, a worker attempts to acquire a lock and waits on a
 * `std::condition_variable`. The thread sleeps efficiently until it is either notified of a new
 * task or the pool is instructed to shut down. This wait-notify mechanism is crucial for
 * preventing busy-waiting and minimizing CPU consumption when the task queue is empty.
 *
 * @param name_prefix A string prefix used to generate unique names for the worker threads
 *                    (e.g., "Worker" results in "Worker-0", "Worker-1", etc.).
 * @param threads The total number of worker threads to create and maintain in the pool.
 */
ThreadPool::ThreadPool(const std::string &name_prefix, size_t threads) : stop_(false) {
    // Pre-allocating the vector's capacity is a performance optimization that avoids
    // potential reallocations and moves of `std::thread` objects during the loop.
    workers_.reserve(threads);

    for (size_t i = 0; i < threads; ++i) {
        // Generate a unique and descriptive name for the underlying OS-level thread. This greatly
        // simplifies debugging and performance analysis in complex systems.
        std::string worker_name = name_prefix + "-" + std::to_string(i);

        workers_.emplace_back([this, worker_name] {
            // Immediately set the thread's name upon its creation for superior diagnostics.
            set_current_thread_name(worker_name);

            // This is the main execution loop for each worker thread in the pool.
            while (true) {
                std::function<void()> task;

                {  // This block defines the critical section for accessing the shared task queue.
                    std::unique_lock<std::mutex> lock(this->queue_mutex_);

                    // The thread waits on the condition variable. The wait is predicated on a
                    // lambda that checks for the stop signal or a non-empty queue. This predicate
                    // is essential for correctly handling spurious wakeups.
                    this->condition_.wait(lock, [this] {
                        return this->stop_.load(std::memory_order_relaxed) || !this->tasks_.empty();
                    });

                    // The exit condition for the worker thread: the pool is stopping, and all
                    // pending tasks have been processed. This ensures a graceful shutdown.
                    if (this->stop_.load(std::memory_order_relaxed) && this->tasks_.empty()) {
                        return;
                    }

                    // Dequeue the next available task from the front of the queue. `std::move`
                    // is used for efficiency.
                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }  // The mutex is automatically released here by the `unique_lock` destructor.

                // The task is executed outside the critical section. This is a critical design
                // choice that maximizes concurrency by allowing other worker threads to access the
                // queue while the current task is being processed.
                task();
            }
        });
    }
}

/**
 * @brief Destroys the `ThreadPool`, ensuring a graceful and orderly shutdown.
 *
 * @details The destructor orchestrates a multi-step shutdown process to guarantee that all
 * submitted work is completed and all thread resources are cleanly released.
 * 1.  It first acquires a lock and sets the atomic `stop_` flag to `true`. This action
 *     prevents any new tasks from being enqueued and signals to the worker threads that
 *     they should begin terminating.
 * 2.  It then broadcasts a notification on the condition variable (`condition_.notify_all()`).
 *     This is crucial to wake up any worker threads that are currently sleeping and waiting for
 * tasks.
 * 3.  Finally, it iterates through the `workers_` vector and calls `join()` on each thread.
 *     This blocks the destructor until each worker thread has finished its current task,
 *     checked the `stop_` flag, and exited its main loop. This guarantees that the `ThreadPool`
 *     object is not fully destroyed until all worker activity has ceased.
 */
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_.store(true, std::memory_order_relaxed);
    }

    // Wake up all threads, including those that may be waiting on the condition variable, so
    // they can re-evaluate the stop condition and begin their exit process.
    condition_.notify_all();

    // Block and wait for each worker thread to complete its execution. This ensures that
    // all resources are properly released and no threads are left running detachably.
    for (std::thread &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

}  // namespace aevum::util::concurrency

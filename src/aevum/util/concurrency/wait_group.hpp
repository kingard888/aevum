// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file wait_group.hpp
 * @brief Defines the `WaitGroup` synchronization primitive, a powerful tool for coordinating
 * the completion of multiple concurrent operations.
 * @details This header file provides the declaration of the `WaitGroup` class. This utility is
 * analogous to Golang's `sync.WaitGroup` and is indispensable for barrier-style synchronization,
 * where a master thread must wait for a specified number of worker threads or asynchronous tasks
 * to conclude their execution before proceeding.
 */
#pragma once

#include <condition_variable>
#include <mutex>

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
 * @class WaitGroup
 * @brief A synchronization primitive that enables one or more threads to wait until a collection
 *        of concurrent operations has completed.
 *
 * @details A `WaitGroup` maintains an internal counter that tracks the number of outstanding
 * operations. The typical usage pattern involves a master thread that:
 * 1. Calls `add(N)` to declare that it will wait for `N` operations to complete.
 * 2. Dispatches `N` worker threads or asynchronous tasks.
 *
 * Each worker, upon finishing its execution, must call the `done()` method, which atomically
 * decrements the internal counter. Meanwhile, the master thread can call `wait()` to block
 * its own execution. The `wait()` call will suspend the thread efficiently until the internal
 * counter is decremented to zero, at which point it is unblocked. This mechanism provides a
 * simple yet powerful way to implement fork-join parallelism.
 */
class WaitGroup {
  public:
    /**
     * @brief Constructs a new `WaitGroup` with its internal counter initialized to zero.
     * @details The `noexcept` specification guarantees that the constructor will not throw,
     * ensuring safe and predictable initialization.
     */
    WaitGroup() noexcept = default;

    /**
     * @brief Destroys the `WaitGroup`.
     * @warning It is critical to ensure that no other threads are actively blocked in `wait()`
     *          when the `WaitGroup` is destroyed. Destroying a `WaitGroup` while it is being
     *          waited upon constitutes a logical error and results in undefined behavior. The
     *          caller is responsible for ensuring proper synchronization during teardown.
     */
    ~WaitGroup() = default;

    /**
     * @brief Deleted copy constructor.
     * @details `WaitGroup` instances are non-copyable to prevent ambiguous state and race
     * conditions that would arise from duplicating the internal counter and synchronization
     * primitives. Each `WaitGroup` must have a unique identity.
     * @param other The `WaitGroup` to be copied (operation is deleted).
     */
    WaitGroup(const WaitGroup &) = delete;

    /**
     * @brief Deleted copy assignment operator.
     * @details Assigning one `WaitGroup` to another is prohibited to maintain clear and
     * predictable synchronization semantics.
     * @param other The `WaitGroup` to be assigned from (operation is deleted).
     * @return A reference to this instance (operation is deleted).
     */
    WaitGroup &operator=(const WaitGroup &) = delete;

    /**
     * @brief Deleted move constructor.
     * @details Moving a `WaitGroup` is disallowed because its address may be used by waiting
     * threads, and moving it would invalidate their synchronization state, leading to
     * undefined behavior.
     * @param other The `WaitGroup` to be moved (operation is deleted).
     */
    WaitGroup(WaitGroup &&) = delete;

    /**
     * @brief Deleted move assignment operator.
     * @details Move assignment is prohibited to ensure the stability and integrity of the
     * synchronization object's lifecycle.
     * @param other The `WaitGroup` to be move-assigned from (operation is deleted).
     * @return A reference to this instance (operation is deleted).
     */
    WaitGroup &operator=(WaitGroup &&) = delete;

    /**
     * @brief Atomically adds a specified value to the internal `WaitGroup` counter.
     *
     * @details This method is typically invoked by a master thread before dispatching worker
     * tasks to register the number of operations that must be completed. A positive `count`
     * increases the number of expected `done()` calls, while a negative `count` can be used
     * for more complex synchronization patterns, though this is less common.
     *
     * @param count The integer value to add to the internal counter. This is typically a
     *              positive number representing the quantity of new tasks. Defaults to 1.
     * @throws `std::invalid_argument` if the operation results in the internal counter becoming
     *         negative, which signifies a logical error in the application's synchronization code.
     */
    void add(int count = 1);

    /**
     * @brief Atomically decrements the `WaitGroup` counter by one.
     *
     * @details This method is intended to be called by a worker thread or at the completion of an
     * asynchronous operation to signal that it has finished its task. When the internal counter
     * reaches zero as a result of this call, any threads that are currently blocked in `wait()`
     * will be unblocked.
     *
     * @throws `std::logic_error` if `done()` is invoked when the counter is already at or below
     *         zero, as this indicates a logical flaw (i.e., more `done()` calls than `add()`
     * calls).
     */
    void done();

    /**
     * @brief Blocks the calling thread until the `WaitGroup` counter becomes zero.
     *
     * @details If the counter is already zero when `wait()` is called, the method returns
     * immediately without blocking. Otherwise, the calling thread is suspended in an
     * efficient waiting state via a `std::condition_variable`. The thread will only be woken
     * up when the counter is decremented to zero by other threads calling `done()`. This method
     * correctly handles spurious wakeups.
     */
    void wait();

  private:
    /**
     * @var counter_
     * @brief The internal integer counter that tracks the number of outstanding operations.
     * Access to this variable is strictly synchronized.
     */
    int counter_{0};

    /**
     * @var mutex_
     * @brief A `std::mutex` that provides exclusive, synchronized access to the `counter_`
     * and is used in conjunction with the `cv_`.
     */
    mutable std::mutex mutex_;

    /**
     * @var cv_
     * @brief A `std::condition_variable` that allows threads to wait efficiently for the
     * `counter_` to reach zero without busy-waiting.
     */
    std::condition_variable cv_;
};

}  // namespace aevum::util::concurrency

// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file wait_group.cpp
 * @brief Implements the `WaitGroup` synchronization primitive for coordinating concurrent tasks.
 * @details This source file provides the concrete definitions for the `WaitGroup` class methods.
 * It details the logic for atomically modifying the internal counter and for orchestrating the
 * blocking and waking of threads, ensuring correct and efficient barrier synchronization.
 */
#include "aevum/util/concurrency/wait_group.hpp"

#include <mutex>
#include <stdexcept>

namespace aevum::util::concurrency {

/**
 * @brief Atomically modifies the internal task counter by a specified delta.
 * @details This function acquires an exclusive lock to safely update the counter, ensuring that
 * all modifications are atomic. It is the primary mechanism for registering or deregistering
 * tasks with the `WaitGroup`. After modifying the counter, it checks if the count has reached
 * zero; if so, it broadcasts a notification to unblock all waiting threads. It also provides
 * critical safety checks to prevent logical errors.
 *
 * @param count The integer value to add to the counter. This value can be positive (to register
 *              new tasks) or negative.
 * @throws `std::invalid_argument` if the counter becomes negative after the addition. A negative
 *         counter is a programmatic error, typically indicating an imbalance between `add()`
 *         and `done()` calls.
 */
void WaitGroup::add(int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    counter_ += count;

    // A negative counter signifies a logical bug in the client code, such as calling `done()`
    // more times than tasks were added. This check provides immediate feedback on such errors.
    if (counter_ < 0) {
        throw std::invalid_argument("WaitGroup counter cannot be negative.");
    }

    // If the counter reaches exactly zero, it signifies that the last registered task has
    // completed. In this case, we must notify all threads that are currently blocked in `wait()`.
    if (counter_ == 0) {
        cv_.notify_all();
    }
}

/**
 * @brief Atomically decrements the internal counter by one, signifying task completion.
 * @details This is a convenience function that is semantically equivalent to `add(-1)`. It is
 * the standard method for a worker thread to signal that it has finished its designated task.
 * The implementation includes a crucial precondition check to ensure that `done()` is not
 * called on a `WaitGroup` that is already in a completed state (i.e., counter is zero or less).
 *
 * @throws `std::logic_error` if the counter is already zero or negative before the decrement.
 *         This indicates a misuse of the `WaitGroup`, such as a superfluous `done()` call.
 */
void WaitGroup::done() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (counter_ <= 0) {
        throw std::logic_error("WaitGroup done() called on a non-positive counter.");
    }

    --counter_;

    // When the final task is completed, the counter transitions to zero. A broadcast
    // notification is sent to wake up all threads that are blocked in the `wait()` method.
    if (counter_ == 0) {
        cv_.notify_all();
    }
}

/**
 * @brief Blocks the calling thread until the internal counter becomes zero.
 * @details This method is the core blocking mechanism of the `WaitGroup`. If the counter is
 * already zero upon invocation, it returns immediately. Otherwise, it atomically releases the
 * internal mutex and suspends the thread's execution, placing it on a wait list associated
 * with the condition variable. The thread will remain in an efficient sleep state until another
 * thread makes the counter zero and broadcasts a notification. The use of a predicate lambda
 * (`[this]() { return counter_ == 0; }`) is essential to protect against spurious wakeups,
 * ensuring the thread only proceeds when the condition is truly met.
 */
void WaitGroup::wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    // The `wait` method on the condition variable will only return when the supplied predicate
    // lambda evaluates to `true`, thus robustly handling any spurious wakeups from the OS.
    cv_.wait(lock, [this]() {
        return counter_ == 0;
    });
}

}  // namespace aevum::util::concurrency

// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file spinlock.hpp
 * @brief Formally defines a high-performance, low-latency spinlock synchronization primitive.
 * @details This header provides the declaration for the `Spinlock` class, a lightweight mutex
 * alternative that employs a busy-waiting (or spinning) strategy. It is engineered for scenarios
 * where lock contention is expected to be low and critical sections are exceedingly short, thereby
 * making the overhead of a kernel-level context switch (as seen with `std::mutex`) undesirable.
 */
#pragma once

#include <atomic>

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
 * @class Spinlock
 * @brief Implements a low-level, high-performance synchronization primitive that utilizes
 * a busy-waiting loop.
 *
 * @details A `Spinlock` repeatedly and atomically tests a lock variable until it becomes available,
 * thereby avoiding the significant performance overhead associated with operating system-level
 * thread suspension and rescheduling. This characteristic renders it exceptionally efficient for
 * protecting very brief, non-blocking critical sections where the expected wait duration is less
 * than the time required for a context switch. The implementation is based on `std::atomic_flag`,
 * which is guaranteed to be lock-free, ensuring maximum efficiency and portability.
 *
 * @note This class is designed to satisfy the `BasicLockable` and `Lockable` named requirements
 * from the C++ standard library. Consequently, it can be seamlessly integrated with standard RAII
 * lock wrappers such as `std::lock_guard` and `std::unique_lock` to ensure exception-safe
 * lock management.
 */
class Spinlock {
  public:
    /**
     * @brief Initializes the spinlock to a default, unlocked (clear) state.
     * @details The constructor leverages `ATOMIC_FLAG_INIT` to guarantee that the underlying
     * `std::atomic_flag` is initialized to a clear state before any operations are performed.
     * The `noexcept` specification guarantees that the constructor will not throw any exceptions.
     */
    Spinlock() noexcept;

    /**
     * @brief Default destructor.
     * @details The destructor requires no special actions, as the underlying `std::atomic_flag`
     * manages its own state.
     */
    ~Spinlock() = default;

    /**
     * @brief Deleted copy constructor.
     * @details `Spinlock` instances are non-copyable. Copying a lock would lead to ambiguous
     * ownership and unpredictable behavior in a multithreaded context, violating the fundamental
     * principle of a unique lock owner.
     * @param other The `Spinlock` to be copied (operation is deleted).
     */
    Spinlock(const Spinlock &) = delete;

    /**
     * @brief Deleted copy assignment operator.
     * @details Assigning one `Spinlock` to another is prohibited to prevent the logical errors
     * and race conditions that would arise from duplicating lock state.
     * @param other The `Spinlock` to be assigned from (operation is deleted).
     * @return A reference to the current instance (operation is deleted).
     */
    Spinlock &operator=(const Spinlock &) = delete;

    /**
     * @brief Acquires the lock, spinning (busy-waiting) until it becomes available.
     * @details This method executes a tight loop that continuously attempts to atomically set the
     * lock flag. If contention is high or the critical section is long, this may consume
     * considerable CPU resources. It is therefore optimally employed for critical sections with
     * minimal and predictable execution times. The operation uses appropriate memory ordering
     * to ensure correctness.
     * @see lock()
     */
    void lock() noexcept;

    /**
     * @brief Attempts to acquire the lock once without spinning.
     * @details This method performs a single, non-blocking attempt to acquire the lock. It is
     * useful in scenarios where a thread can perform alternative work if the lock is not
     * immediately available, thus preventing wasted CPU cycles from spinning.
     * @return `true` if the lock was successfully acquired, and `false` if it is currently held
     * by another thread.
     * @see try_lock()
     */
    [[nodiscard]] bool try_lock() noexcept;

    /**
     * @brief Releases the lock, allowing another spinning thread to acquire it.
     * @details This operation atomically clears the lock flag, making it available for other
     * threads. It is imperative that `unlock()` is called only by the thread that currently
     * holds the lock. Failure to do so results in undefined behavior.
     * @see unlock()
     */
    void unlock() noexcept;

  private:
    /**
     * @var flag_
     * @brief The core `std::atomic_flag` that represents the state of the lock.
     * @details `std::atomic_flag` is specified by the C++ standard as the premier type for
     * building synchronization primitives. It is guaranteed to be lock-free and provides the
     * necessary atomic test-and-set operations, making it the ideal foundation for an efficient
     * and portable spinlock.
     */
    std::atomic_flag flag_;
};

}  // namespace aevum::util::concurrency

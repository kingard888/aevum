// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file spinlock.cpp
 * @brief Implements the high-performance, low-latency Spinlock synchronization primitive.
 * @details This source file provides the concrete implementation of the `Spinlock` class methods.
 * It details the atomic operations and memory ordering semantics required to create a correct
 * and efficient busy-wait lock. The implementation prioritizes performance for short-duration
 * critical sections in highly concurrent environments.
 */
#include "aevum/util/concurrency/spinlock.hpp"

#include <thread>

namespace aevum::util::concurrency {

/**
 * @brief Constructs the Spinlock.
 * @details This constructor initializes the underlying `std::atomic_flag` to a clear (unlocked)
 * state. The `ATOMIC_FLAG_INIT` macro provides a standardized, static initialization that
 * guarantees the flag is in a known, available state before any locking attempts are made.
 * This initialization is fundamental to the spinlock's correctness.
 */
Spinlock::Spinlock() noexcept : flag_(ATOMIC_FLAG_INIT) {}

/**
 * @brief Acquires the lock by engaging in a busy-wait (spin) loop until successful.
 * @details This method implements the core spinning mechanism. It repeatedly invokes `test_and_set`
 * on the atomic flag. This operation atomically sets the flag to `true` and returns its previous
 * state. The loop continues as long as the flag was already set (i.e., the lock is held).
 *
 * To ensure correct memory visibility, `std::memory_order_acquire` is used. This memory fence
 * guarantees that all memory read and write operations following the lock acquisition in the
 * program order will not be reordered to occur before it. This is critical for ensuring that
 * modifications made within the critical section by other threads are visible after the lock
 * is acquired.
 *
 * To mitigate excessive CPU consumption and prevent live-locking on certain architectures,
 * `std::this_thread::yield()` is called within the loop. This politely suggests to the operating
 * system scheduler that the current thread's time slice can be relinquished, allowing other
 * threads to execute.
 */
void Spinlock::lock() noexcept {
    // The `test_and_set` operation is an atomic read-modify-write (RMW) instruction. It returns
    // `true` if the flag was already set, indicating contention. The loop continues until
    // `test_and_set` returns `false`, signifying that this thread has successfully acquired the
    // lock.
    while (flag_.test_and_set(std::memory_order_acquire)) {
        // Yielding the processor prevents the current thread from monopolizing a CPU core while
        // waiting, which is particularly important on single-core systems or under heavy load.
        std::this_thread::yield();

        // AevumDB maintainer note: In scenarios of extreme contention on x86/x64 architectures,
        // one might consider replacing `yield()` with the `_mm_pause()` intrinsic (or its
        // equivalent). This instruction signals to the processor that the code is in a spin-wait
        // loop, which can improve performance by reducing power consumption and avoiding costly
        // pipeline flushes upon exiting the loop. This is a micro-optimization and should be
        // employed judiciously.
    }
}

/**
 * @brief Attempts to acquire the lock once without blocking or spinning.
 * @details This method provides a non-blocking mechanism to acquire the lock. It performs a single
 * `test_and_set` operation. If the lock is acquired (`test_and_set` returns `false`), the method
 * returns `true`. If the lock is already held (`test_and_set` returns `true`), it returns `false`
 * immediately. This is highly valuable in contexts where a thread can perform alternative, useful
 * work instead of waiting. The `std::memory_order_acquire` fence is used to ensure the same
 * memory visibility guarantees as the `lock()` method upon successful acquisition.
 * @return `true` if the lock was successfully acquired; `false` otherwise.
 */
bool Spinlock::try_lock() noexcept {
    // The return value of `test_and_set` reflects the state of the flag *before* the set operation.
    // Therefore, a `false` return indicates that the lock was previously free and has now been
    // acquired by this call. We negate this result to align with the expected `true`-on-success
    // semantics of a `try_lock` function.
    return !flag_.test_and_set(std::memory_order_acquire);
}

/**
 * @brief Releases a previously acquired lock.
 * @details This method atomically clears the `std::atomic_flag`, setting its state to `false` and
 * thus making the lock available for other threads to acquire.
 *
 * It is crucial that this operation uses `std::memory_order_release`. This memory fence ensures
 * that all memory writes and reads that occurred within the critical section (before this unlock
 * call) are completed and visible to other threads before the lock is released. This prevents
 * reordering of memory operations and guarantees that any thread subsequently acquiring the lock
 * will see a consistent state.
 */
void Spinlock::unlock() noexcept { flag_.clear(std::memory_order_release); }

}  // namespace aevum::util::concurrency

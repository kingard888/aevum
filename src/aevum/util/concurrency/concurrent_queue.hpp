// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file concurrent_queue.hpp
 * @brief Articulates the formal definition of a thread-safe, generic queue, engineered for
 * high-throughput concurrent applications.
 * @details This header file furnishes a sophisticated, templated queue implementation that
 * guarantees atomicity and thread safety for all its operations. It is an essential component for
 * designing robust, multithreaded systems that rely on message passing or task distribution, such
 * as in producer-consumer patterns or parallel processing pipelines.
 */
#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>

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
 * @class ConcurrentQueue
 * @brief Implements a fully thread-safe, generic, first-in-first-out (FIFO) data structure,
 * meticulously designed for high-performance producer-consumer scenarios.
 *
 * @details This class provides a powerful abstraction over a standard `std::queue` by integrating
 * fine-grained locking mechanisms using a `std::mutex` and efficient thread coordination via a
 * `std::condition_variable`. This design ensures serialized access to the underlying container,
 * thereby preventing data races and maintaining data structure integrity in highly-concurrent
 * environments. The class is engineered to minimize lock contention and to obviate the need for
 * busy-waiting, leading to superior CPU efficiency and system performance.
 *
 * @tparam T The type of elements to be stored in the queue. This type must be movable to support
 * efficient element transfer.
 */
template <typename T>
class ConcurrentQueue {
  public:
    /**
     * @brief Constructs a new, empty `ConcurrentQueue` instance.
     * @details The default constructor initializes the internal synchronization primitives and the
     * underlying queue container.
     */
    ConcurrentQueue() = default;

    /**
     * @brief Destroys the `ConcurrentQueue` object.
     * @details The destructor ensures that all internal resources are properly released. Any
     * elements remaining in the queue upon destruction are also destroyed.
     */
    ~ConcurrentQueue() = default;

    /**
     * @brief Deleted copy constructor to prevent unintended copying of the queue's state.
     * @details Copying a `ConcurrentQueue` is disallowed as it would involve complex and ambiguous
     * semantics regarding the state of the mutex, condition variable, and the contained elements,
     * potentially leading to deadlocks or data corruption.
     * @param other The `ConcurrentQueue` to be copied (operation is deleted).
     */
    ConcurrentQueue(const ConcurrentQueue &) = delete;

    /**
     * @brief Deleted copy assignment operator to prevent unsafe state overwrites.
     * @details Assigning one `ConcurrentQueue` to another is prohibited to maintain the integrity
     * of the synchronization primitives and prevent resource management conflicts.
     * @param other The `ConcurrentQueue` to be assigned from (operation is deleted).
     * @return A reference to the current instance (operation is deleted).
     */
    ConcurrentQueue &operator=(const ConcurrentQueue &) = delete;

    /**
     * @brief Deleted move constructor to ensure stable memory and synchronization state.
     * @details Moving a `ConcurrentQueue` is disabled because the internal synchronization
     * mechanisms are intrinsically tied to the object's identity and memory location.
     * @param other The `ConcurrentQueue` to be moved (operation is deleted).
     */
    ConcurrentQueue(ConcurrentQueue &&) = delete;

    /**
     * @brief Deleted move assignment operator for the same reasons as the move constructor.
     * @details Move assignment is disallowed to prevent the logical inconsistencies and potential
     * race conditions that could arise from transferring the queue's state.
     * @param other The `ConcurrentQueue` to be move-assigned from (operation is deleted).
     * @return A reference to the current instance (operation is deleted).
     */
    ConcurrentQueue &operator=(ConcurrentQueue &&) = delete;

    /**
     * @brief Atomically enqueues an element to the back of the queue.
     * @details This operation acquires a unique lock on the internal mutex, performs the push
     * operation on the underlying `std::queue`, and then notifies exactly one waiting consumer
     * thread that a new item has become available. The element is passed by value and moved
     * internally to optimize performance by avoiding unnecessary copies.
     * @param value The element to be added to the queue.
     */
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(value));
        }
        condition_.notify_one();
    }

    /**
     * @brief Attempts to dequeue an element from the front of the queue without blocking.
     * @details This method acquires a lock and checks if the queue is non-empty. If an element
     * is available, it is moved out of the queue and returned within a `std::optional`. If the
     * queue is empty, the method returns `std::nullopt` immediately, without waiting. This
     * non-blocking behavior is ideal for polling scenarios.
     * @return An `std::optional<T>` that contains the dequeued element if the operation was
     * successful, or `std::nullopt` if the queue was empty.
     */
    [[nodiscard]] std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }

        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    /**
     * @brief Atomically dequeues an element, blocking the calling thread until one is available.
     * @details This method employs a `std::unique_lock` in conjunction with a
     * `std::condition_variable`. If the queue is empty, the thread is put into a waiting state,
     * relinquishing the lock to avoid blocking other threads. The wait is predicated on the queue
     * becoming non-empty, which robustly handles spurious wakeups. Upon notification and successful
     * reacquisition of the lock, it dequeues and returns the front element.
     * @return The element from the front of the queue.
     */
    [[nodiscard]] T wait_and_pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] {
            return !queue_.empty();
        });

        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    /**
     * @brief Atomically checks whether the queue is empty.
     * @details This operation acquires a lock to safely inspect the state of the underlying
     * container, providing a snapshot-in-time of the queue's emptiness.
     * @return `true` if the queue contains no elements at the moment of the check; `false`
     * otherwise.
     */
    [[nodiscard]] bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * @brief Atomically returns the number of elements currently stored in the queue.
     * @details This operation acquires a lock to safely query the size of the underlying
     * container, yielding a consistent view of the queue's current depth.
     * @return The number of elements in the queue as a `size_t`.
     */
    [[nodiscard]] size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /**
     * @brief Atomically removes all elements from the queue.
     * @details This method acquires a lock and clears the queue's contents in a highly efficient
     * and memory-safe manner by swapping the internal queue with a newly created empty queue.
     * This idiom ensures that the memory from the old elements is released outside the lock's
     * critical section, minimizing lock-holding time.
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::queue<T> empty_queue;
        std::swap(queue_, empty_queue);
    }

  private:
    /**
     * @var queue_
     * @brief The underlying `std::queue` instance that stores the elements. This is the core
     * data container managed by the class.
     */
    std::queue<T> queue_;

    /**
     * @var mutex_
     * @brief A mutable `std::mutex` that provides exclusive, synchronized access to the `queue_`.
     * It is marked `mutable` to allow locking in `const` member functions like `empty()` and
     * `size()`.
     */
    mutable std::mutex mutex_;

    /**
     * @var condition_
     * @brief A `std::condition_variable` used to orchestrate the blocking and waking of threads.
     * It enables consumer threads in `wait_and_pop` to sleep efficiently until a producer thread
     * signals the availability of a new element via `push`.
     */
    std::condition_variable condition_;
};

}  // namespace aevum::util::concurrency

// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file object_pool.hpp
 * @brief Defines a thread-safe, generic object pool for high-efficiency object recycling.
 * @details This header provides the `ObjectPool` class, a sophisticated memory management utility
 * that reduces the overhead of frequent dynamic memory allocation and deallocation. By maintaining
 * a cache of pre-allocated, reusable objects, it is ideal for performance-critical applications
 * that repeatedly create and destroy objects of the same type.
 */
#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "aevum/util/concurrency/spinlock.hpp"

namespace aevum::util::memory {

/**
 * @class ObjectPool
 * @brief A thread-safe, generic pool designed for the efficient recycling of frequently used
 * objects.
 *
 * @details This class provides a powerful mechanism to mitigate the significant performance cost
 * associated with frequent dynamic memory operations (`new` and `delete`). It maintains an internal
 * cache (a "pool") of pre-allocated objects. When an object is requested via `acquire()`, the pool
 * first attempts to provide a recycled object from its cache. If no objects are available, it
 * allocates a new one. When an object is no longer needed, it can be returned to the pool via
 * `release()` for subsequent reuse, avoiding the deallocation and potential heap fragmentation.
 *
 * To ensure correctness in multithreaded environments, all access to the internal object cache is
 * synchronized using a low-latency `aevum::util::concurrency::Spinlock`. This makes the pool
 * particularly suitable for high-performance, highly concurrent applications where contention on
 * the global heap allocator could become a bottleneck.
 *
 * @tparam T The type of object to be managed by the pool. This type `T` must be
 * default-constructible to allow the pool to create new instances when necessary.
 */
template <typename T>
class ObjectPool {
  public:
    /**
     * @brief Constructs an empty `ObjectPool`.
     * @details The constructor initializes the internal storage and synchronization primitives.
     * The pool starts with no cached objects.
     */
    ObjectPool() = default;

    /**
     * @brief Destroys the `ObjectPool` and deallocates all cached objects within it.
     * @details The destructor acquires a lock to ensure thread-safe cleanup, then iterates through
     * all objects currently stored in the pool and calls `delete` on each one, ensuring that all
     * memory is returned to the system.
     */
    ~ObjectPool() {
        std::lock_guard<aevum::util::concurrency::Spinlock> lock(spinlock_);
        for (T *obj : pool_) {
            delete obj;
        }
        pool_.clear();
    }

    // The copy and move semantics are explicitly deleted. An `ObjectPool` has unique ownership of
    // the objects it contains. Allowing copies or moves would create ambiguity over which pool is
    // responsible for managing and deleting the objects, inevitably leading to memory leaks or
    // double-free corruption.
    ObjectPool(const ObjectPool &) = delete;
    ObjectPool &operator=(const ObjectPool &) = delete;
    ObjectPool(ObjectPool &&) = delete;
    ObjectPool &operator=(ObjectPool &&) = delete;

    /**
     * @brief Acquires an object from the pool, creating one if necessary.
     * @details This method is the primary way to obtain an object. It first attempts to retrieve a
     *          recycled object from the internal cache. If the cache is empty, it dynamically
     *          allocates a new object of type `T` on the heap. The entire operation is performed
     *          atomically under the protection of a spinlock.
     * @return A non-null pointer to an initialized object of type `T`. The caller assumes
     *         ownership of this pointer until it is returned via `release()`.
     */
    [[nodiscard]] T *acquire() {
        std::lock_guard<aevum::util::concurrency::Spinlock> lock(spinlock_);
        if (pool_.empty()) {
            return new T();
        }

        T *obj = pool_.back();
        pool_.pop_back();
        return obj;
    }

    /**
     * @brief Releases an object, returning it to the pool for future reuse.
     * @details The provided object pointer is relinquished by the caller and returned to the pool's
     *          internal cache. The caller must not access the object through this pointer after
     *          the call, as its ownership has been transferred back to the pool. The operation is
     *          thread-safe.
     * @param obj A pointer to the object to be returned to the pool. If `nullptr` is provided,
     *            the function has no effect and safely returns.
     * @note For objects that hold state, it is the caller's responsibility to reset the object to a
     *       clean state before releasing it, or alternatively, the `release` method could be
     * modified to call a `reset()` method on the object before caching it.
     */
    void release(T *obj) {
        if (obj == nullptr) return;

        std::lock_guard<aevum::util::concurrency::Spinlock> lock(spinlock_);
        pool_.push_back(obj);
    }

    /**
     * @brief Atomically retrieves the number of available objects currently cached in the pool.
     * @details This method provides a thread-safe way to query the current number of recycled
     * objects available for immediate acquisition without allocation.
     * @return The number of available objects as a `size_t`.
     */
    [[nodiscard]] size_t available_count() const noexcept {
        std::lock_guard<aevum::util::concurrency::Spinlock> lock(spinlock_);
        return pool_.size();
    }

  private:
    /**
     * @var pool_
     * @brief A vector that stores pointers to the available, recycled objects. It acts as a LIFO
     * stack, as objects are pushed to the back and popped from the back.
     */
    std::vector<T *> pool_;

    /**
     * @var spinlock_
     * @brief A mutable spinlock that ensures thread-safe, serialized access to the `pool_` vector.
     * It is marked `mutable` to be lockable within the `const` `available_count` method.
     */
    mutable aevum::util::concurrency::Spinlock spinlock_;
};

}  // namespace aevum::util::memory

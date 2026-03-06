// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file arena_allocator.hpp
 * @brief Defines a high-performance, arena-based memory allocator for bulk object management.
 * @details This header file specifies the `ArenaAllocator` class, a memory management utility
 * designed for scenarios involving the allocation of many small objects with similar lifetimes.
 * It operates by pre-allocating large, contiguous memory blocks (chunks or arenas) and serves
 * allocation requests by simply advancing a pointer, a technique that dramatically outperforms
 * standard heap allocators (`malloc`/`new`) by avoiding per-object overhead and reducing heap
 * fragmentation.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

/**
 * @namespace aevum::util::memory
 * @brief Provides components for specialized and high-performance memory management strategies.
 * @details This namespace contains utilities that offer alternatives to standard heap allocation,
 * targeting specific use cases like object pooling and region-based memory management to improve
 * application performance and predictability.
 */
namespace aevum::util::memory {

/**
 * @class ArenaAllocator
 * @brief A fast, region-based (arena) memory allocator for managing object lifetimes collectively.
 *
 * @details The `ArenaAllocator` provides a significant performance advantage over standard dynamic
 * allocation by drastically reducing the number of expensive calls to the operating system's memory
 * manager. It pre-allocates large, contiguous blocks of memory (chunks) and fulfills subsequent
 * smaller allocation requests by simply advancing a pointer within the current chunk. This "bump
 * pointer" approach is exceptionally fast and eliminates the overhead associated with managing
 * individual memory blocks on the heap, thereby mitigating heap fragmentation.
 *
 * Deallocation of all objects within the arena is an extremely fast, constant-time O(1) operation
 * that simply resets the internal pointers. This makes arenas ideal for phased computations where
 * all objects created during a phase can be discarded simultaneously.
 *
 * @warning A critical characteristic of this allocator is that it does not call destructors for
 * the objects it manages when the arena is reset or destroyed. Therefore, it is best suited for
 * Plain Old Data (POD) types or for objects that do not manage external resources (like file
 * handles or network sockets) and whose lifetimes are strictly contained within the arena's scope.
 * Misuse with complex objects can lead to resource leaks.
 */
class ArenaAllocator {
  public:
    /**
     * @brief Constructs an `ArenaAllocator` and pre-allocates the first memory chunk.
     * @param chunk_size The size in bytes of each large memory block to be allocated from the OS.
     *                   This value should be chosen based on the expected memory usage patterns.
     *                   A larger chunk size reduces the frequency of OS allocations but may waste
     *                   memory if usage is low. It defaults to 64KB, a common and effective size.
     */
    explicit ArenaAllocator(size_t chunk_size = 64 * 1024);

    /**
     * @brief Destroys the `ArenaAllocator`, releasing all of its allocated memory chunks back to
     * the OS.
     * @details The destructor safely deallocates all memory blocks that were acquired from the heap
     * during the allocator's lifetime.
     */
    ~ArenaAllocator() = default;

    // The `ArenaAllocator` is made non-copyable and non-movable. This is a critical design choice
    // to maintain pointer stability and prevent complex lifetime and ownership issues. If an arena
    // were moved, all pointers to objects allocated within it would be invalidated.
    ArenaAllocator(const ArenaAllocator &) = delete;
    ArenaAllocator &operator=(const ArenaAllocator &) = delete;
    ArenaAllocator(ArenaAllocator &&) = delete;
    ArenaAllocator &operator=(ArenaAllocator &&) = delete;

    /**
     * @brief Allocates a block of raw, uninitialized memory from the arena.
     *
     * @details This is the core allocation function. It returns a pointer to a block of memory
     * satisfying the requested size and alignment. The allocation is served from the current chunk
     * if space is available; otherwise, a new chunk is allocated. The method correctly handles
     * alignment by calculating and adding the necessary padding.
     *
     * @param bytes The number of bytes to allocate.
     * @param alignment The required memory alignment for the allocated block, specified in bytes.
     *                  This must be a power of two. Defaults to the maximum fundamental alignment
     *                  supported by the platform (`alignof(std::max_align_t)`).
     * @return A non-null pointer to the beginning of the allocated memory block.
     * @throws `std::bad_alloc` if the requested size exceeds the chunk size or if a new chunk
     *         cannot be allocated from the OS.
     */
    [[nodiscard]] void *allocate(size_t bytes, size_t alignment = alignof(std::max_align_t));

    /**
     * @brief A convenience method to allocate memory and construct an object in-place within the
     * arena.
     *
     * @details This function elegantly combines memory allocation and object construction. It first
     * allocates a suitable block of memory using `allocate()` and then uses a placement-new
     * expression to construct an object of type `T` directly at that memory location, perfectly
     * forwarding the provided arguments to its constructor.
     *
     * @tparam T The type of the object to be created.
     * @tparam Args The types of the arguments to be forwarded to `T`'s constructor.
     * @param args The arguments to pass to the constructor of `T`.
     * @return A pointer to the newly constructed object of type `T`.
     */
    template <typename T, typename... Args>
    [[nodiscard]] T *create(Args &&...args) {
        void *ptr = allocate(sizeof(T), alignof(T));
        return new (ptr) T(std::forward<Args>(args)...);
    }

    /**
     * @brief Resets the allocator, effectively deallocating all memory within the arena for reuse.
     *
     * @details This is a constant-time O(1) operation that makes the entire allocated region
     * available for new allocations by simply resetting the internal chunk index and offset
     * pointers. It does not release memory back to the operating system and, crucially, does
     * not invoke destructors on any objects previously created in the arena.
     */
    void reset() noexcept;

  private:
    /**
     * @brief Allocates a new memory chunk from the heap and adds it to the arena's pool.
     * @details This internal helper function is invoked by the constructor or when the current
     * memory chunk is exhausted.
     */
    void add_chunk();

    /**
     * @var chunk_size_
     * @brief The prescribed size in bytes for each new memory chunk allocated by the arena.
     */
    size_t chunk_size_;

    /**
     * @var chunks_
     * @brief A vector of smart pointers, each managing the lifetime of a large, heap-allocated
     * memory chunk. `std::unique_ptr<std::byte[]>` is used for correct, exception-safe memory
     * management.
     */
    std::vector<std::unique_ptr<std::byte[]>> chunks_;

    /**
     * @var current_chunk_idx_
     * @brief The index within the `chunks_` vector of the chunk currently being used for
     * allocations.
     */
    size_t current_chunk_idx_{0};

    /**
     * @var current_offset_
     * @brief The byte offset within the current chunk. This acts as the "bump pointer," indicating
     * the start of the next available free memory.
     */
    size_t current_offset_{0};
};

}  // namespace aevum::util::memory

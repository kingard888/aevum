// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file arena_allocator.cpp
 * @brief Implements the `ArenaAllocator` class for high-performance, region-based memory
 * management.
 * @details This source file provides the concrete definitions for the `ArenaAllocator` methods.
 * It details the logic for chunk management, alignment calculation, and the "bump-pointer"
 * allocation strategy that makes this allocator highly efficient.
 */
#include "aevum/util/memory/arena_allocator.hpp"

#include <algorithm>
#include <new>

namespace aevum::util::memory {

/**
 * @brief Constructs an `ArenaAllocator` and allocates its initial memory chunk.
 * @param chunk_size The size in bytes for each large memory block (chunk) that the arena will
 *                   manage. This size is fixed for the lifetime of the allocator.
 */
ArenaAllocator::ArenaAllocator(size_t chunk_size) : chunk_size_(chunk_size) { add_chunk(); }

/**
 * @brief Allocates a block of memory from the current chunk, handling alignment and
 * chunk-switching.
 * @details This method implements the core "bump-pointer" allocation logic. It first calculates the
 * padding required to ensure that the returned pointer meets the specified alignment requirement.
 * If the current chunk has insufficient space for the requested size plus padding, it advances to
 * the next available chunk. If no more chunks are available, it allocates a new one. A request
 * that is larger than the chunk size itself will result in an exception.
 *
 * @param bytes The number of bytes to allocate.
 * @param alignment The required memory alignment for the allocation.
 * @return A `void*` pointer to the start of the newly allocated, properly aligned block of memory.
 * @throws `std::bad_alloc` if a new chunk cannot be allocated from the system or if the requested
 *         allocation size (including alignment padding) exceeds the `chunk_size_`.
 */
void *ArenaAllocator::allocate(size_t bytes, size_t alignment) {
    // The padding calculation is a common and efficient bit of pointer arithmetic. It determines
    // the number of bytes needed to advance the current offset to the next address that is a
    // multiple of `alignment`. The outer modulo handles the case where the offset is already
    // aligned.
    size_t padding = (alignment - (current_offset_ % alignment)) % alignment;
    size_t required_space = bytes + padding;

    // Check if the current chunk can satisfy the request.
    if (current_offset_ + required_space > chunk_size_) {
        // A single allocation cannot be larger than the chunk size. This is a design limitation
        // of this simple arena. More advanced implementations might handle this by allocating a
        // special, oversized chunk just for this request.
        if (required_space > chunk_size_) {
            throw std::bad_alloc();
        }

        // If the current chunk is full, advance to the next one.
        current_chunk_idx_++;
        // If we have exhausted all existing chunks, allocate a new one.
        if (current_chunk_idx_ >= chunks_.size()) {
            add_chunk();
        }

        // After switching to a new chunk, reset the offset and recalculate padding.
        current_offset_ = 0;
        padding = (alignment - (current_offset_ % alignment)) % alignment;
        required_space = bytes + padding;
    }

    // Apply the alignment padding by advancing the offset.
    current_offset_ += padding;

    // The final, aligned pointer is the base address of the current chunk plus the new offset.
    void *ptr = chunks_[current_chunk_idx_].get() + current_offset_;

    // Advance the offset by the number of bytes requested to "claim" the memory block.
    current_offset_ += bytes;

    return ptr;
}

/**
 * @brief Resets the allocator, making all its memory available for immediate reuse.
 * @details This is an extremely fast, O(1) operation. It invalidates all previously allocated
 * pointers by simply resetting the internal state, effectively moving the allocation pointer
 * back to the beginning of the very first chunk. No memory is deallocated or returned to the
 * operating system, but the entire arena becomes ready for a new wave of allocations.
 */
void ArenaAllocator::reset() noexcept {
    current_chunk_idx_ = 0;
    current_offset_ = 0;
}

/**
 * @brief Allocates a new memory chunk from the heap and adds it to the arena's list of chunks.
 * @details This helper function is called when the arena is first constructed or when an
 * allocation request cannot be satisfied by any of the existing chunks. It uses
 * `std::make_unique<std::byte[]>` to perform a heap allocation, ensuring that the memory
 * is correctly managed and will be automatically released when the `ArenaAllocator` is destroyed.
 * @throws `std::bad_alloc` if the underlying system memory allocation fails.
 */
void ArenaAllocator::add_chunk() { chunks_.push_back(std::make_unique<std::byte[]>(chunk_size_)); }

}  // namespace aevum::util::memory

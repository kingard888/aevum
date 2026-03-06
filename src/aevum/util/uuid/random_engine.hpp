// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file random_engine.hpp
 * @brief Defines the interface for a thread-local, high-quality random number generator engine.
 * @details This header file declares the `next_uint64` function, which serves as the core source
 * of randomness for UUID generation. The implementation is specifically designed to be both
 * high-quality, using a Mersenne Twister engine seeded with hardware entropy, and highly
 * performant in concurrent scenarios by leveraging thread-local storage to eliminate lock
 * contention.
 */
#pragma once

#include <cstdint>

namespace aevum::util::uuid::detail {

/**
 * @brief Generates a cryptographically-strong, uniformly-distributed 64-bit unsigned integer.
 *
 * @details This function provides a robust and highly efficient source of randomness, which is
 * critical for generating high-quality version 4 UUIDs. It employs a `thread_local` storage
 * strategy to instantiate a separate random number generation suite for each thread. This suite
 * consists of:
 * 1. `std::random_device`: A non-deterministic generator used to source hardware entropy,
 *    providing a high-quality seed.
 * 2. `std::mt19937_64`: The 64-bit Mersenne Twister engine, a fast and high-quality
 *    pseudo-random number generator, which is seeded by the `std::random_device`.
 * 3. `std::uniform_int_distribution`: Ensures that the output values are uniformly distributed
 *    across the entire range of `uint64_t`.
 *
 * By making these components `thread_local`, this function entirely avoids the need for locks or
 * other synchronization primitives, eliminating contention and making it exceptionally fast in
 * highly concurrent applications.
 *
 * @return A random `uint64_t` value with a uniform distribution. The `noexcept` specification
 *         guarantees that this function will not throw exceptions.
 */
[[nodiscard]] uint64_t next_uint64() noexcept;

}  // namespace aevum::util::uuid::detail

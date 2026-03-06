// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file fnv1a.hpp
 * @brief Declares the interface for the 64-bit Fowler-Noll-Vo (FNV-1a) non-cryptographic
 * hashing algorithm.
 * @details This header provides the function prototype for generating a 64-bit hash value using
 * the FNV-1a algorithm. FNV is designed for high speed and is particularly well-suited for
 * producing a hash value from a stream of data. The '1a' variant offers improved avalanche
 * characteristics over the original FNV-1.
 */
#pragma once

#include <cstdint>
#include <string_view>

/**
 * @namespace aevum::util::hash
 * @brief A collection of fast, non-cryptographic hashing algorithms.
 * @details This namespace provides implementations of various hashing utilities that are optimized
 * for speed and are suitable for use in data structures like hash tables, but not for
 * security-sensitive applications.
 */
namespace aevum::util::hash {

/**
 * @brief Computes the 64-bit FNV-1a hash of a given block of data.
 *
 * @details This function implements the Fowler-Noll-Vo hash, variant 1a. The FNV hash is an
 * exceptionally fast non-cryptographic hashing algorithm, designed for high dispersion and
 * minimal collisions in hash table implementations. The '1a' variant differs from the original
 * FNV-1 by reversing the order of the XOR and multiplication operations within its main loop.
 * This change results in better avalanche behavior, meaning small changes in the input data
 * lead to significantly different output hashes.
 *
 * The algorithm proceeds as follows:
 * 1. Initialize the hash to the 64-bit FNV offset basis (`0xcbf29ce484222325`).
 * 2. For each byte in the input data:
 *    a. XOR the hash with the byte's value.
 *    b. Multiply the hash by the 64-bit FNV prime (`0x100000001b3`).
 *
 * @param data A `std::string_view` representing the contiguous block of data to be hashed.
 *             The use of `std::string_view` ensures that no unnecessary copies of the input
 *             data are made, enhancing performance.
 * @return The calculated 64-bit unsigned hash value. The function is guaranteed not to throw
 *         exceptions and is marked `noexcept`.
 */
[[nodiscard]] uint64_t fnv1a_64(std::string_view data) noexcept;

}  // namespace aevum::util::hash

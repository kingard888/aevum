// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file djb2.hpp
 * @brief Declares the interface for the DJB2 non-cryptographic hashing algorithm.
 * @details This header provides the function prototypes for generating a 64-bit hash value using
 * the DJB2 algorithm, a simple yet effective hash function known for its speed and good
 * distribution properties, particularly with string data.
 */
#pragma once

#include <cstdint>
#include <string>
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
 * @brief Computes the 64-bit DJB2 hash of a given block of data.
 *
 * @details This function implements the DJB2 algorithm, conceived by Daniel J. Bernstein. It is a
 * non-cryptographic hash function renowned for its simplicity, speed, and excellent distribution
 * of hash values for common inputs, making it an ideal choice for hash table implementations.
 * The algorithm iteratively processes each byte of the input data, updating the hash value
 * according to the formula: `hash = ((hash << 5) + hash) + c`, which is equivalent to
 * `hash = hash * 33 + c`.
 *
 * @param data A `std::string_view` representing the contiguous block of data to be hashed.
 *             Using `std::string_view` avoids unnecessary memory allocations and copies by
 *             providing a non-owning view of the character sequence.
 * @return The calculated 64-bit unsigned hash value. The `noexcept` specification guarantees
 *         that this function will not throw exceptions.
 */
[[nodiscard]] uint64_t djb2(std::string_view data) noexcept;

/**
 * @brief Computes the DJB2 hash and formats the result as a decimal string.
 *
 * @details This convenience function first calculates the 64-bit numeric hash of the input data
 * using the `djb2` function and then converts this integer value into its corresponding
 * `std::string` representation. This is particularly useful for applications requiring a
 * string-based key or identifier derived from the data's hash.
 *
 * @param data The input data to be hashed, provided as a `std::string_view`.
 * @return A `std::string` containing the decimal representation of the 64-bit hash value.
 */
[[nodiscard]] std::string djb2_string(std::string_view data);

}  // namespace aevum::util::hash

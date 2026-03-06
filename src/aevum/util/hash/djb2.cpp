// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file djb2.cpp
 * @brief Provides the concrete implementation of the DJB2 non-cryptographic hashing algorithm.
 * @details This source file defines the functions for generating a 64-bit hash value using
 * the DJB2 algorithm. The implementation is optimized for performance through bitwise arithmetic.
 */
#include "aevum/util/hash/djb2.hpp"

namespace aevum::util::hash {

/**
 * @brief Computes the 64-bit DJB2 hash for a given contiguous block of data.
 * @details This function provides a high-performance implementation of the DJB2 algorithm, which
 * was created by Professor Dan Bernstein. It is celebrated for its excellent distribution and
 * speed, particularly with short strings, making it a stellar choice for hash table
 * implementations. The algorithm begins with a magic initial seed value of 5381. It then iterates
 * through each character of the input data, updating the hash value using the formula `hash = (hash
 * * 33) + c`. This multiplication by 33 is efficiently implemented using a bitwise left shift
 * (`hash << 5`) and an addition, which is significantly faster on most processor architectures than
 * a direct multiplication instruction.
 *
 * @param data A `std::string_view` providing a non-owning reference to the data that needs to be
 *             hashed. This choice of parameter type avoids costly string copy operations.
 * @return The computed 64-bit unsigned hash value. This function is marked `noexcept` as it
 *         consists of purely arithmetic operations and is guaranteed not to throw exceptions.
 */
uint64_t djb2(std::string_view data) noexcept {
    // The value 5381 is the traditional starting seed for the DJB2 algorithm. Its specific origin
    // is historical, but it has been proven to contribute to the algorithm's good distribution.
    uint64_t hash = 5381;

    for (char c : data) {
        // This line is the core of the DJB2 algorithm. It is a highly optimized equivalent of:
        // hash = (hash * 33) + c;
        // The expression `(hash << 5)` computes `hash * 32`. Adding `hash` once more yields
        // `hash * 33`. This bitwise arithmetic is substantially faster than multiplication.
        hash = ((hash << 5) + hash) + static_cast<uint64_t>(c);
    }

    return hash;
}

/**
 * @brief Computes the DJB2 hash and returns its decimal string representation.
 * @details This function serves as a convenient wrapper that first computes the numeric hash
 * value by calling `djb2(data)` and subsequently converts this 64-bit integer into its
 * standard `std::string` form. This is useful in scenarios where the hash is needed as a
 * string-based key, such as in certain caching or distributed systems.
 *
 * @param data A `std::string_view` of the data to be hashed.
 * @return A `std::string` that contains the decimal representation of the computed hash.
 */
std::string djb2_string(std::string_view data) { return std::to_string(djb2(data)); }

}  // namespace aevum::util::hash

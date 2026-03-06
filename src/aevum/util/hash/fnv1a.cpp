// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file fnv1a.cpp
 * @brief Provides the concrete implementation of the 64-bit Fowler-Noll-Vo (FNV-1a) hashing
 * algorithm.
 * @details This source file contains the function definition for `fnv1a_64`, which computes a
 * 64-bit hash value based on the FNV-1a specification. The implementation uses magic numbers
 * (an offset basis and a prime) defined by the FNV standard.
 */
#include "aevum/util/hash/fnv1a.hpp"

namespace aevum::util::hash {

/**
 * @brief Computes the 64-bit FNV-1a hash for a given contiguous block of data.
 * @details This function implements the Fowler-Noll-Vo '1a' variant, a non-cryptographic hash
 * algorithm engineered for high speed and excellent distribution properties. The '1a' variant
 * is generally preferred over the original FNV-1 due to its superior avalanche characteristics,
 * which means small perturbations in the input data result in drastically different output hashes.
 *
 * The algorithm is initialized with a standard 64-bit offset basis. It then iterates through each
 * byte of the input data, applying the core FNV-1a transformation: first, it XORs the current
 * hash value with the byte, and then it multiplies the result by the standard 64-bit FNV prime
 * number. This sequence of operations is what distinguishes it from FNV-1 and contributes to its
 * improved hashing properties.
 *
 * @param data A `std::string_view` offering a non-owning, read-only view of the data to be hashed.
 *             This is highly efficient as it obviates the need for memory allocation or data
 * copying.
 * @return The computed 64-bit unsigned hash value. The `noexcept` specifier asserts that this
 *         function will not throw any exceptions, as it is based purely on arithmetic operations.
 */
uint64_t fnv1a_64(std::string_view data) noexcept {
    // The FNV offset basis is a magic number specified by the FNV-1a standard for 64-bit hashes.
    // It is the initial value of the hash before processing any data.
    uint64_t hash = 0xcbf29ce484222325ULL;

    // The FNV prime is another magic number from the standard. Its properties are chosen to
    // help ensure good distribution of the final hash value across the 64-bit space.
    constexpr uint64_t prime = 0x100000001b3ULL;

    for (char c : data) {
        // This is the core transformation of the FNV-1a algorithm.
        // 1. The current hash is XORed with the current byte of data. The `static_cast` ensures
        //    the character is treated as an unsigned byte value.
        hash ^= static_cast<uint8_t>(c);
        // 2. The result is then multiplied by the FNV prime. This multiplication spreads the
        //    bits of the hash value, contributing to the avalanche effect.
        hash *= prime;
    }

    return hash;
}

}  // namespace aevum::util::hash

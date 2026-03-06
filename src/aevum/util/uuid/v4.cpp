// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file v4.cpp
 * @brief Implements the generation logic for Version 4 (randomly-generated) UUIDs.
 * @details This source file provides the concrete definition for the `generate_v4` function.
 * The implementation focuses on high performance by using a contention-free random number
 * source, efficient bitwise operations for setting the version and variant fields, and a direct,
 * high-speed hexadecimal formatting approach.
 */
#include "aevum/util/uuid/v4.hpp"

#include <array>
#include <string>

#include "aevum/util/uuid/random_engine.hpp"

namespace aevum::util::uuid {

/**
 * @brief Generates a new, standards-compliant, randomly-generated (Version 4) UUID.
 *
 * @details This function orchestrates the creation of a Version 4 UUID according to RFC 4122.
 * The process involves three main steps, each optimized for performance and correctness:
 *
 * 1.  **Random Number Generation**: Two 64-bit random numbers are obtained from the thread-local,
 *     contention-free `detail::next_uint64()` engine. This provides the 128 bits of entropy
 *     required for the UUID.
 *
 * 2.  **Version and Variant-Setting**: Specific bits within the 128-bit random data are overwritten
 *     to conform to the RFC 4122 standard for a Version 4, Variant 1 UUID.
 *     - The 4-bit version field is set to `0100` (binary for 4).
 *     - The 2-bit variant field is set to `10` (binary for the RFC 4122 variant).
 *     This is accomplished using efficient bitwise AND and OR operations with pre-calculated masks.
 *
 * 3.  **Hexadecimal Formatting**: The final 128 bits are formatted into the canonical
 *     36-character hexadecimal string representation (e.g.,
 * `xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx`). This implementation avoids slower, stream-based
 * formatting in favor of a direct, character-by- character construction using a `constexpr` lookup
 * table for hexadecimal characters, which can be highly optimized by the compiler.
 *
 * @return A `std::string` containing the newly generated UUIDv4.
 */
std::string generate_v4() {
    // Step 1: Obtain 128 bits of high-quality random data.
    uint64_t part1 = detail::next_uint64();
    uint64_t part2 = detail::next_uint64();

    // Step 2: Apply bitmasks to set the UUID version and variant fields.
    // Set the version to 4 (0100) in the most significant 4 bits of the 7th byte.
    // The mask 0x0FFF retains the other bits, and 0x4000 sets the version.
    part1 = (part1 & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    // Set the variant to 1 (10xx) in the most significant 2 bits of the 9th byte.
    // The mask 0x3FFF... retains the other bits, and 0x8000... sets the variant.
    part2 = (part2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    // Step 3: Perform high-performance hexadecimal conversion and formatting.
    // A `constexpr` lookup table allows the compiler to heavily optimize the conversion.
    static constexpr std::array<char, 16> hex_chars = {'0', '1', '2', '3', '4', '5', '6', '7',
                                                       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    // Pre-allocating the string with hyphens in place avoids reallocations and simplifies
    // formatting.
    std::string uuid(36, '-');

    // A helper lambda for converting a 4-bit nibble to its hexadecimal character representation.
    auto to_hex = [&](uint64_t val, int shift) constexpr -> char {
        return hex_chars[(val >> shift) & 0x0F];
    };

    // Unroll the formatting loop for maximum performance. Each character of the UUID is
    // directly calculated and placed, avoiding conditional branches and complex formatting logic.

    // Format the first group (8 hex chars).
    uuid[0] = to_hex(part1, 60);
    uuid[1] = to_hex(part1, 56);
    uuid[2] = to_hex(part1, 52);
    uuid[3] = to_hex(part1, 48);
    uuid[4] = to_hex(part1, 44);
    uuid[5] = to_hex(part1, 40);
    uuid[6] = to_hex(part1, 36);
    uuid[7] = to_hex(part1, 32);
    // uuid[8] is a hyphen.

    // Format the second group (4 hex chars).
    uuid[9] = to_hex(part1, 28);
    uuid[10] = to_hex(part1, 24);
    uuid[11] = to_hex(part1, 20);
    uuid[12] = to_hex(part1, 16);
    // uuid[13] is a hyphen.

    // Format the third group (4 hex chars), which includes the version nibble.
    uuid[14] = to_hex(part1, 12);
    uuid[15] = to_hex(part1, 8);
    uuid[16] = to_hex(part1, 4);
    uuid[17] = to_hex(part1, 0);
    // uuid[18] is a hyphen.

    // Format the fourth group (4 hex chars), which includes the variant nibble.
    uuid[19] = to_hex(part2, 60);
    uuid[20] = to_hex(part2, 56);
    uuid[21] = to_hex(part2, 52);
    uuid[22] = to_hex(part2, 48);
    // uuid[23] is a hyphen.

    // Format the final group (12 hex chars).
    uuid[24] = to_hex(part2, 44);
    uuid[25] = to_hex(part2, 40);
    uuid[26] = to_hex(part2, 36);
    uuid[27] = to_hex(part2, 32);
    uuid[28] = to_hex(part2, 28);
    uuid[29] = to_hex(part2, 24);
    uuid[30] = to_hex(part2, 20);
    uuid[31] = to_hex(part2, 16);
    uuid[32] = to_hex(part2, 12);
    uuid[33] = to_hex(part2, 8);
    uuid[34] = to_hex(part2, 4);
    uuid[35] = to_hex(part2, 0);

    return uuid;
}

}  // namespace aevum::util::uuid

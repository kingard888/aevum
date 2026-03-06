// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file v4.hpp
 * @brief Defines the interface for generating Version 4 (randomly-generated) UUIDs.
 * @details This header file provides the declaration for the `generate_v4` function, a utility
 * for creating universally unique identifiers that are based on high-quality random numbers,
 * in conformance with RFC 4122.
 */
#pragma once

#include <string>

namespace aevum::util::uuid {

/**
 * @brief Generates a new, random, Version 4 Universally Unique Identifier (UUID).
 *
 * @details This function constructs a UUID of Version 4, which is derived entirely from random
 * numbers. The generation process conforms strictly to RFC 4122, which mandates specific bits
 * to be set to identify the UUID version and variant. The output is a 36-character string in the
 * canonical `xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx` format, where '4' indicates Version 4, and 'y'
 * (one of '8', '9', 'a', or 'b') indicates the RFC 4122 variant.
 *
 * The implementation is highly optimized for performance, using a thread-local random number
 * generator to avoid lock contention and employing efficient bitwise operations and direct
 * hexadecimal character mapping to format the final string.
 *
 * @return A `std::string` containing the newly generated, standards-compliant UUIDv4.
 */
[[nodiscard]] std::string generate_v4();

}  // namespace aevum::util::uuid

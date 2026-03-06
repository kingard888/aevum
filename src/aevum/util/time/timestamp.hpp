// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file timestamp.hpp
 * @brief Defines a set of utility functions for generating standardized system timestamps.
 * @details This header file provides the interfaces for acquiring the current system time in
 * two common, highly useful formats: as a UNIX timestamp in milliseconds and as a human-readable
 * ISO 8601 string in UTC. These utilities are essential for logging, event ordering, and data
 * serialization.
 */
#pragma once

#include <cstdint>
#include <string>

namespace aevum::util::time {

/**
 * @brief Retrieves the current system time as a UNIX epoch timestamp, measured in milliseconds.
 *
 * @details This function calculates the total number of milliseconds that have elapsed since the
 * UNIX epoch (00:00:00 Coordinated Universal Time (UTC), Thursday, 1 January 1970). This format
 * is a language-agnostic and time-zone-independent standard, making it exceptionally well-suited
 * for globally ordering events, storing timestamps in databases, and for use in distributed
 * systems.
 *
 * @return The current time as a `uint64_t`, representing the number of milliseconds since the
 * epoch. The `noexcept` specification guarantees this function will not throw exceptions.
 */
[[nodiscard]] uint64_t now_unix_ms() noexcept;

/**
 * @brief Retrieves the current system time and formats it as an ISO 8601 string in UTC.
 *
 * @details This function provides a standardized, human-readable string representation of the
 * current time. The output strictly conforms to the ISO 8601 format (e.g., "YYYY-MM-DDTHH:MM:SSZ"),
 * which is unambiguous and widely recognized, making it ideal for logging, data interchange formats
 * like JSON, and any context where a precise, universally understood timestamp is required. The 'Z'
 * suffix explicitly denotes UTC (Zulu time).
 *
 * @return A `std::string` containing the formatted UTC datetime. The implementation handles
 *         platform-specific, thread-safe time conversion.
 */
[[nodiscard]] std::string now_iso8601();

}  // namespace aevum::util::time

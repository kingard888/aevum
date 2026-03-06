// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file timestamp.cpp
 * @brief Implements the concrete logic for generating standardized system timestamps.
 * @details This source file provides the definitions for timestamp generation functions. It uses
 * the C++ `<chrono>` library for time acquisition and includes platform-specific code to
 * ensure thread-safe conversion to UTC calendar time for formatting.
 */
#include "aevum/util/time/timestamp.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace aevum::util::time {

/**
 * @brief Retrieves the current system time as a UNIX epoch timestamp in milliseconds.
 * @details This function leverages `std::chrono::system_clock`, which is designed to represent
 * wall-clock time. It obtains the current time point, calculates its duration since the clock's
 * epoch (which is typically the UNIX epoch), and then casts this duration to milliseconds. The
 * result is a 64-bit unsigned integer, which is sufficient to represent timestamps for millennia
 * without overflow.
 *
 * @return The number of milliseconds that have elapsed since 00:00:00 UTC on 1 January 1970.
 */
uint64_t now_unix_ms() noexcept {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

/**
 * @brief Retrieves the current system time and formats it as a UTC-based ISO 8601 string.
 * @details This function first obtains the current time from `std::chrono::system_clock`. It then
 * converts this high-resolution time point into a `std::time_t` value, which is a more traditional
 * calendar time representation. A critical step is the conversion of this `time_t` into a
 * broken-down time structure (`std::tm`) corresponding to UTC. This is handled using
 * platform-specific, thread-safe functions (`gmtime_s` on Windows, `gmtime_r` on POSIX systems) to
 * avoid the data races associated with the non-thread-safe `std::gmtime`. Finally, `std::put_time`
 * is used to format the `std::tm` struct into the standard "YYYY-MM-DDTHH:MM:SSZ" format.
 *
 * @return A `std::string` containing the fully formatted, null-terminated UTC datetime string.
 */
std::string now_iso8601() {
    auto now = std::chrono::system_clock::now();
    std::time_t time_now_t = std::chrono::system_clock::to_time_t(now);

    // Create a struct to hold the broken-down UTC time.
    std::tm tm_utc{};

// Use platform-specific thread-safe functions for UTC conversion. `gmtime` is not thread-safe
// as it often uses a static internal buffer.
#if defined(_WIN32) || defined(_WIN64)
    // On Windows, `gmtime_s` is the thread-safe equivalent.
    gmtime_s(&tm_utc, &time_now_t);
#else
    // On POSIX-compliant systems (like Linux and macOS), `gmtime_r` is the reentrant (thread-safe)
    // version.
    gmtime_r(&time_now_t, &tm_utc);
#endif

    // Use an `ostringstream` and `std::put_time` for robust and locale-independent formatting.
    std::ostringstream ss;
    ss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

}  // namespace aevum::util::time

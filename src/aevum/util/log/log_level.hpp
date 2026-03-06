// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file log_level.hpp
 * @brief Defines the enumerated type for message severity within the logging framework.
 * @details This header file establishes the `LogLevel` scoped enumeration, which categorizes log
 * messages by their severity. It also provides a compile-time utility function to convert these
 * severity levels into human-readable string representations, facilitating clear and
 * informative log output.
 */
#pragma once

#include <cstdint>
#include <string_view>

namespace aevum::util::log {

/**
 * @enum LogLevel
 * @brief Defines a set of strongly-typed, ordered severity levels for log messages.
 * @details The enumeration is scoped (`enum class`) to prevent name clashes and allow for
 * stronger type checking. The underlying type is specified as `uint8_t` for memory efficiency.
 * The levels are ordered by increasing severity, allowing for simple filtering logic
 * (e.g., `level >= LogLevel::WARN`).
 */
enum class LogLevel : uint8_t {
    /**
     * @var DEBUG
     * @brief Verbose, fine-grained information intended for developers during debugging.
     * These messages are typically suppressed in production builds to reduce performance overhead.
     */
    DEBUG = 0,
    /**
     * @var INFO
     * @brief General, informational messages that highlight the normal progress and state of the
     * application. Useful for tracing high-level application flow.
     */
    INFO = 1,
    /**
     * @var WARN
     * @brief Indicates a potential issue or an unexpected event that does not disrupt the
     * application's primary functionality but may warrant investigation.
     */
    WARN = 2,
    /**
     * @var ERROR
     * @brief Signifies that a recoverable error has occurred. The application can continue
     * operation, but some functionality may be degraded or a specific operation may have failed.
     */
    ERROR = 3,
    /**
     * @var FATAL
     * @brief Represents a critical, non-recoverable error that will likely lead to the
     * termination of the application. These messages indicate a severe failure.
     */
    FATAL = 4
};

/**
 * @brief Converts a `LogLevel` enumerator to its corresponding string representation.
 * @details This is a `constexpr` function, allowing the conversion to occur at compile time
 * where possible, which enhances performance. It returns a `std::string_view` to avoid
 * unnecessary memory allocations. If an invalid `LogLevel` value is provided, it returns
 * a default "UNKNOWN" string.
 *
 * @param level The `LogLevel` value to be converted.
 * @return A `std::string_view` literal representing the log level (e.g., "DEBUG", "INFO").
 *         The function is marked `noexcept` as it is guaranteed not to throw exceptions.
 */
[[nodiscard]] constexpr std::string_view to_string(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO ";
        case LogLevel::WARN:
            return "WARN ";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}

}  // namespace aevum::util::log

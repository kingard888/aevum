// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file logger.cpp
 * @brief Implements the concrete logic for the thread-safe, static logging engine.
 * @details This source file provides the definitions for the `Logger` class's static methods.
 * It manages a global log level and employs a global mutex to ensure that log messages from
 * multiple threads are written atomically to the appropriate output streams (`std::cout` or
 * `std::cerr`), preventing interleaved and corrupted output. The log output is enhanced with
 * color-coding and includes timestamps and thread IDs for comprehensive diagnostics.
 */
#include "aevum/util/log/logger.hpp"

#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

#include "aevum/util/time/timestamp.hpp"

namespace aevum::util::log {

// A file-static global variable that holds the current minimum log level.
// Messages with a severity below this level will be discarded. It is initialized
// to `LogLevel::INFO` as a sensible default for most applications.
static LogLevel g_current_level = LogLevel::INFO;

// A file-static global mutex to protect against data races when multiple threads
// attempt to write to the standard output streams simultaneously. This ensures that
// each log message is written as a single, contiguous block.
static std::mutex g_log_mutex;

// ANSI escape codes for coloring log output in the terminal.
namespace colors {
constexpr std::string_view RESET = "\033[0m";
constexpr std::string_view BLUE = "\033[34m";
constexpr std::string_view GREEN = "\033[32m";
constexpr std::string_view YELLOW = "\033[33m";
constexpr std::string_view RED = "\033[31m";
constexpr std::string_view BOLDRED = "\033[1;31m";
}  // namespace colors

/**
 * @brief Sets the global minimum log level threshold.
 * @details This function provides a thread-safe way to dynamically control the verbosity of the
 * logger. It updates the `g_current_level` variable, which is then used by the `write`
 * function to filter messages.
 * @param level The new minimum `LogLevel` to apply for all subsequent logging calls.
 */
void Logger::set_level(LogLevel level) noexcept {
    // This operation is simple and atomic on most platforms for `uint8_t`,
    // but explicit synchronization would be required for more complex state.
    g_current_level = level;
}

/**
 * @brief The core function for writing a formatted, color-coded log message if its level meets the
 * threshold.
 * @details This is the centralized sink for all log messages. It performs the following sequence
 * of operations in a thread-safe manner:
 * 1. It first performs a quick check against the global log level. If the message's severity
 *    is below the threshold, the function returns immediately.
 * 2. It generates a current timestamp, the string representation of the level, and the current
 * thread ID.
 * 3. It acquires a `std::lock_guard` on the global mutex for exclusive access to the console
 * output.
 * 4. It selects the appropriate output stream (`stderr` for errors, `stdout` for others).
 * 5. It selects an ANSI color code based on the severity level.
 * 6. It writes the fully formatted log message, including timestamp, level, thread ID, and content.
 * 7. For `FATAL` errors, it explicitly flushes the stream to guarantee visibility before a crash.
 * @param level The severity level of the message being logged.
 * @param message The content of the log message.
 */
void Logger::write(LogLevel level, std::string_view message) noexcept {
    // Discard the message immediately if its level is below the configured threshold.
    if (level < g_current_level) return;

    // Generate the components of the log entry.
    std::string timestamp = aevum::util::time::now_iso8601();
    std::string_view level_str = to_string(level);
    std::stringstream thread_id_ss;
    thread_id_ss << std::this_thread::get_id();

    const std::string_view *color;
    switch (level) {
        case LogLevel::DEBUG:
            color = &colors::BLUE;
            break;
        case LogLevel::INFO:
            color = &colors::GREEN;
            break;
        case LogLevel::WARN:
            color = &colors::YELLOW;
            break;
        case LogLevel::ERROR:
            color = &colors::RED;
            break;
        case LogLevel::FATAL:
            color = &colors::BOLDRED;
            break;
        default:
            color = &colors::RESET;
            break;
    }

    // Acquire a lock to ensure the integrity of the output stream.
    std::lock_guard<std::mutex> lock(g_log_mutex);

    // Direct error and fatal messages to stderr, others to stdout.
    std::ostream &out = (level >= LogLevel::ERROR) ? std::cerr : std::cout;

    out << "[" << timestamp << "] " << *color << "[" << level_str << "]" << colors::RESET << " ["
        << thread_id_ss.str() << "] " << message << "\n";

    if (level == LogLevel::FATAL) {
        out.flush();
    }
}

}  // namespace aevum::util::log

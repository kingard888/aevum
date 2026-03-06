// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file logger.hpp
 * @brief Defines the interface for a static, globally-accessible, thread-safe logging utility.
 * @details This header file declares the `Logger` class, which provides a simple yet powerful
 * static interface for application-wide logging. It is designed as a utility class (i.e., not
 * meant to be instantiated) and offers a set of methods for logging messages at various
 * severity levels.
 */
#pragma once

#include <string_view>

#include "aevum/util/log/log_level.hpp"

namespace aevum::util::log {

/**
 * @class Logger
 * @brief Provides a simple, globally accessible, and thread-safe interface for logging.
 *
 * @details This class is designed as a pure static utility and, as such, all its member
 * functions are static and it cannot be instantiated. It offers a centralized and straightforward
 * mechanism for emitting log messages from anywhere in the application. The logging operations
 * are internally synchronized to prevent interleaved or corrupted output when called from
 * multiple threads simultaneously. Messages are routed to either `stdout` or `stderr` based on
 * their severity level, a common practice for separating informational output from error
 * diagnostics.
 *
 * Each log entry is meticulously formatted to include critical diagnostic information:
 * - A high-resolution timestamp (ISO 8601 format).
 * - A color-coded severity level, improving visual parsing in terminal environments.
 * - The ID of the thread that generated the log message, crucial for debugging concurrent
 * operations.
 * - The actual log message content.
 *
 * The standard log output format is: `[<timestamp>] [<COLOR_CODE><LEVEL><RESET>] [<thread_id>]
 * <message>`
 */
class Logger {
  public:
    /**
     * @brief Deleted default constructor to prevent instantiation.
     * @details The `Logger` class is a purely static utility class and is not intended to be
     * instantiated. Deleting the constructor enforces this design choice at compile time.
     */
    Logger() = delete;

    /**
     * @brief Sets the global minimum severity level for which log messages will be processed.
     * @details This function allows for dynamic adjustment of the application's logging verbosity.
     * Any log message with a severity level lower than the specified `level` will be efficiently
     * discarded and will not be written to any output stream. This is the primary mechanism
     * for controlling log output in different environments (e.g., DEBUG in development, INFO or
     * WARN in production).
     * @param level The minimum `LogLevel` to be recorded.
     */
    static void set_level(LogLevel level) noexcept;

    /**
     * @brief The core, low-level logging function that formats and writes a message.
     * @details This function is the final destination for all log messages. It is thread-safe,
     * using a global mutex to serialize access to the output streams. Before writing, it
     * prepends a timestamp, thread ID, and the message's severity level string (which is also
     * color-coded). It is not typically called directly; developers should use the level-specific
     * convenience wrappers like `info()`, `warn()`, etc., for better readability and intent.
     * @param level The `LogLevel` severity of the message.
     * @param message The content of the message to be logged.
     */
    static void write(LogLevel level, std::string_view message) noexcept;

    /**
     * @brief Logs a message with `DEBUG` severity.
     * @details A convenience wrapper that invokes `write()` with `LogLevel::DEBUG`.
     * @param msg The message to log, provided as a `std::string_view` for efficiency.
     */
    static void debug(std::string_view msg) noexcept { write(LogLevel::DEBUG, msg); }

    /**
     * @brief Logs a message with `INFO` severity.
     * @details A convenience wrapper that invokes `write()` with `LogLevel::INFO`.
     * @param msg The message to log, provided as a `std::string_view`.
     */
    static void info(std::string_view msg) noexcept { write(LogLevel::INFO, msg); }

    /**
     * @brief Logs a message with `WARN` severity.
     * @details A convenience wrapper that invokes `write()` with `LogLevel::WARN`.
     * @param msg The message to log, provided as a `std::string_view`.
     */
    static void warn(std::string_view msg) noexcept { write(LogLevel::WARN, msg); }

    /**
     * @brief Logs a message with `ERROR` severity.
     * @details A convenience wrapper that invokes `write()` with `LogLevel::ERROR`.
     * @param msg The message to log, provided as a `std::string_view`.
     */
    static void error(std::string_view msg) noexcept { write(LogLevel::ERROR, msg); }

    /**
     * @brief Logs a message with `FATAL` severity.
     * @details A convenience wrapper that invokes `write()` with `LogLevel::FATAL`. A fatal
     * log message will also typically flush the output stream immediately to ensure it is
     * captured before a potential application crash.
     * @param msg The message to log, provided as a `std::string_view`.
     */
    static void fatal(std::string_view msg) noexcept { write(LogLevel::FATAL, msg); }
};

}  // namespace aevum::util::log

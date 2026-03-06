// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file status.hpp
 * @brief Defines the core `Status` class, a foundational component for uniform error handling
 * throughout the system.
 * @details This header file specifies the `Status` class and its associated `StatusCode`
 * enumeration. This system provides a lightweight, explicit, and exception-free mechanism for
 * functions to report detailed success or failure outcomes. It is inspired by similar, proven
 * error-handling patterns in large-scale systems like Google's Abseil and LevelDB.
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

/**
 * @namespace aevum::util
 * @brief Contains fundamental utilities and cross-cutting components that are broadly applicable
 * across the AevumDB codebase.
 */
namespace aevum::util {

/**
 * @enum StatusCode
 * @brief Enumerates a comprehensive set of possible outcome codes for operations that can fail.
 * @details This strongly-typed enumeration provides a machine-readable classification of error
 * conditions, allowing for programmatic branching based on the type of failure.
 */
enum class StatusCode : uint8_t {
    kOk = 0,        ///< The operation completed without any errors.
    kNotFound = 1,  ///< A requested entity (e.g., a file, database record) could not be located.
    kCorruption =
        2,  ///< A data corruption error was detected (e.g., failed checksum, invalid file format).
    kNotSupported = 3,  ///< The requested operation is not implemented or is not supported in the
                        ///< current context.
    kInvalidArgument =
        4,  ///< An argument supplied to a method was invalid (e.g., out of range, null pointer).
    kIOError = 5,  ///< A low-level I/O error occurred (e.g., file system read/write error, network
                   ///< failure).
    kUnauthorized =
        6  ///< The caller lacks the necessary permissions to perform the requested operation.
};

/**
 * @class Status
 * @brief A unified, lightweight type for encapsulating the result of an operation that may fail.
 *
 * @details The `Status` class is a cornerstone of error handling in this project. It promotes a
 * clear and explicit error-handling discipline by requiring callers to check the status of an
 * operation. A `Status` object can represent either success (in which case `ok()` is `true`) or a
 * specific failure, which includes a `StatusCode` and a descriptive error message. This approach
 * avoids the performance and complexity overhead of C++ exceptions for non-exceptional control
 * flow, making it ideal for high-performance systems.
 */
class Status {
  public:
    /**
     * @brief Creates a success status (`kOk`) by default.
     * @details The default constructor creates a `Status` object that represents a successful
     * outcome. This allows for convenient and readable success returns (e.g., `return {};`).
     */
    Status() noexcept : code_(StatusCode::kOk) {}

    /**
     * @brief Creates a failure status with a specific code and a detailed message.
     * @param code The `StatusCode` categorizing the nature of the error.
     * @param msg A human-readable message providing specific details about the error context.
     */
    Status(StatusCode code, std::string_view msg) : code_(code), message_(msg) {}

    // The class is designed to be trivially copyable and movable, allowing it to be returned
    // by value from functions with minimal overhead.
    Status(const Status &) = default;
    Status &operator=(const Status &) = default;
    Status(Status &&) noexcept = default;
    Status &operator=(Status &&) noexcept = default;

    /** @brief A static factory method for explicitly creating a success status. */
    static Status OK() { return Status(); }
    /** @brief A static factory method for creating a `kNotFound` status with a message. */
    static Status NotFound(std::string_view msg) { return Status(StatusCode::kNotFound, msg); }
    /** @brief A static factory method for creating a `kCorruption` status with a message. */
    static Status Corruption(std::string_view msg) { return Status(StatusCode::kCorruption, msg); }
    /** @brief A static factory method for creating an `kInvalidArgument` status with a message. */
    static Status InvalidArgument(std::string_view msg) {
        return Status(StatusCode::kInvalidArgument, msg);
    }
    /** @brief A static factory method for creating a `kNotSupported` status with a message. */
    static Status NotSupported(std::string_view msg) {
        return Status(StatusCode::kNotSupported, msg);
    }
    /** @brief A static factory method for creating an `kIOError` status with a message. */
    static Status IOError(std::string_view msg) { return Status(StatusCode::kIOError, msg); }
    /** @brief A static factory method for creating an `kUnauthorized` status with a message. */
    static Status Unauthorized(std::string_view msg) {
        return Status(StatusCode::kUnauthorized, msg);
    }

    /** @brief Checks if the status represents a successful outcome. @return `true` if code is
     * `kOk`. */
    [[nodiscard]] bool ok() const noexcept { return code_ == StatusCode::kOk; }

    /** @brief Returns the specific, machine-readable error code. */
    [[nodiscard]] StatusCode code() const noexcept { return code_; }

    /** @brief Returns the detailed, human-readable error message. */
    [[nodiscard]] const std::string &message() const noexcept { return message_; }

    /**
     * @brief Composes a detailed string representation of the status for logging or debugging.
     * @return For an OK status, returns "OK". For a failure, returns a string in the format
     *         "ErrorType: Detailed message".
     */
    [[nodiscard]] std::string to_string() const {
        if (ok()) return "OK";
        std::string result;
        switch (code_) {
            case StatusCode::kNotFound:
                result = "Not Found: ";
                break;
            case StatusCode::kCorruption:
                result = "Corruption: ";
                break;
            case StatusCode::kNotSupported:
                result = "Not Supported: ";
                break;
            case StatusCode::kInvalidArgument:
                result = "Invalid Argument: ";
                break;
            case StatusCode::kIOError:
                result = "IO Error: ";
                break;
            case StatusCode::kUnauthorized:
                result = "Unauthorized: ";
                break;
            default:
                result = "Unknown Error: ";
                break;
        }
        result.append(message_);
        return result;
    }

  private:
    /**
     * @var code_
     * @brief The status code that categorizes the outcome of the operation.
     */
    StatusCode code_;

    /**
     * @var message_
     * @brief A string containing a detailed message for failure statuses. It remains empty for OK
     * statuses.
     */
    std::string message_;
};

}  // namespace aevum::util

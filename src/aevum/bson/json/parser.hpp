// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file parser.hpp
 * @brief Defines the interface for a high-level utility to parse JSON strings into BSON `Document`
 * objects.
 * @details This header file declares the `parse` function, which provides a safe and convenient C++
 * wrapper around the underlying `libbson` functionality for converting a JSON text representation
 * into a binary BSON document. It emphasizes safety through the use of `Status` for error
 * reporting and RAII for memory management.
 */
#pragma once

#include <string_view>

#include "aevum/bson/doc/document.hpp"
#include "aevum/util/status.hpp"

/**
 * @namespace aevum::bson::json
 * @brief Provides a suite of utilities for bidirectional conversion between JSON text format and
 * the BSON binary format.
 * @details This namespace encapsulates functions for parsing JSON into BSON (`parse`) and
 * serializing BSON into JSON (`to_string`), bridging the gap between human-readable text and the
 * efficient binary representation.
 */
namespace aevum::bson::json {

/**
 * @brief Parses a JSON-formatted string and converts it into a BSON `Document`.
 *
 * @details This function serves as a robust C++ facade for `libbson`'s JSON parsing engine. It
 * is designed for safety and efficiency, accepting a `std::string_view` to prevent unnecessary
 * string allocations and copies. Upon successful parsing, it populates the provided `out_doc`
 * `Document` object, transferring ownership of the newly created BSON data to this RAII wrapper.
 *
 * Error handling is explicit and robust. If the JSON is malformed, the function will not
 * throw an exception; instead, it will return a `Status::InvalidArgument` object containing a
 * detailed error message captured directly from the underlying `libbson` parser.
 *
 * @param json The input JSON data to be parsed, provided as a `std::string_view`. The view
 *             must point to a valid, null-terminated string for the duration of the call.
 * @param out_doc An output parameter. This reference to a `Document` will be reassigned to hold
 *                the newly parsed BSON data if the operation is successful. The previous content
 *                of `out_doc` (if any) will be safely destroyed.
 * @return Returns `Status::OK()` on successful parsing.
 * @return On failure, returns `Status::InvalidArgument`, with the status message containing a
 *         detailed description of the JSON syntax error.
 */
[[nodiscard]] aevum::util::Status parse(std::string_view json, aevum::bson::doc::Document &out_doc);

}  // namespace aevum::bson::json

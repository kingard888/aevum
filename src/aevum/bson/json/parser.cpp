// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file parser.cpp
 * @brief Implements the JSON-to-BSON parsing functionality.
 * @details This file provides the implementation for converting a JSON formatted
 * string into a `bson_t` object, which is then managed by the
 * `aevum::bson::doc::Document` class.
 */
#include "aevum/bson/json/parser.hpp"

#include <bson/bson.h>

#include "aevum/util/log/logger.hpp"

namespace aevum::bson::json {

/**
 * @brief Parses a JSON string and converts it into a BSON document.
 * @details This function serves as the primary deserialization mechanism from JSON to BSON.
 * It leverages the `bson_new_from_json` function from the underlying `libbson` library,
 * which handles the complex parsing of the JSON string and its conversion into the
 * binary BSON format.
 *
 * Error handling is managed by checking the `bson_error_t` struct that is populated
 * by the `libbson` function. If an error is detected, the details are logged, and the
 * function returns an appropriate error `Status`.
 *
 * @param json_str A `std::string_view` containing the JSON data to be parsed.
 * @param[out] doc A reference to a `aevum::bson::doc::Document` object that will be
 *             populated with the parsed BSON data upon success. The previous content
 *             of this `Document` will be replaced.
 * @return `aevum::util::Status::OK()` if the JSON is successfully parsed and converted
 *         to BSON.
 * @return `aevum::util::Status::InvalidArgument` with a descriptive message if the
 *         JSON string is malformed or if any other parsing error occurs.
 */
aevum::util::Status parse(std::string_view json_str, aevum::bson::doc::Document &doc) {
    bson_error_t error;
    bson_t *b = bson_new_from_json(reinterpret_cast<const uint8_t *>(json_str.data()),
                                   json_str.length(), &error);

    if (!b) {
        std::string error_message =
            "BSON-JSON-Parser: Failed to parse JSON string. Error in domain " +
            std::to_string(error.domain) + " with code " + std::to_string(error.code) + ": " +
            error.message;
        aevum::util::log::Logger::warn(error_message);
        return aevum::util::Status::InvalidArgument(error.message);
    }

    doc = aevum::bson::doc::Document(b);
    return aevum::util::Status::OK();
}

}  // namespace aevum::bson::json

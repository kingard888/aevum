// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file serializer.hpp
 * @brief Defines the interface for a utility to serialize BSON `Document` objects into JSON
 * strings.
 * @details This header file declares the `to_string` function, which provides a high-level C++
 * interface for converting a binary BSON document into its human-readable JSON text
 * representation, specifically using the Relaxed Extended JSON format for maximum compatibility.
 */
#pragma once

#include <string>

#include "aevum/bson/doc/document.hpp"

namespace aevum::bson::json {

/**
 * @brief Serializes a BSON `Document` into its Relaxed Extended JSON string representation.
 *
 * @details This function provides a safe and convenient C++ wrapper for `libbson`'s serialization
 * capabilities. It takes a BSON `Document` and converts its binary data into a human-readable
 * string. The function specifically uses the "Relaxed Extended JSON" format, which balances
 * readability with type fidelity. This format represents standard JSON types (strings, numbers,
 * booleans) directly, while using a special `{"$type": "value"}` syntax for BSON-specific types
 * (e.g., `{"$oid": "..."}` for ObjectIds, `{"$date": {"$numberLong": "..."}}` for UTC datetimes).
 * This ensures that the serialized string can be parsed by standard JSON libraries while
 * retaining the ability to be converted back to BSON without data type loss.
 *
 * @param doc A constant reference to the BSON `Document` to be serialized.
 * @return A `std::string` containing the JSON representation of the document. In the event of an
 *         invalid input document or an internal serialization failure, it returns an empty JSON
 *         object `"{}"` as a safe fallback.
 */
[[nodiscard]] std::string to_string(const aevum::bson::doc::Document &doc);

}  // namespace aevum::bson::json

// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file serializer.cpp
 * @brief Implements the BSON-to-JSON serialization functionality.
 * @details This file contains the implementation for converting a `bson_t` object,
 * as wrapped by the `aevum::bson::doc::Document` class, into its canonical
 * string representation in standard (relaxed) JSON format.
 */
#include "aevum/bson/json/serializer.hpp"

#include <bson/bson.h>
#include <memory>

#include "aevum/util/log/logger.hpp"

namespace aevum::bson::json {

/**
 * @brief Converts a BSON document into its canonical JSON string representation.
 * @details This function serves as the primary serialization mechanism from BSON to JSON.
 * It leverages the `bson_as_canonical_extended_json` function from the underlying
 * `libbson` library, which produces a standardized, human-readable JSON string.
 * The memory allocated by `libbson` for the resulting string is safely managed using
 * a `std::unique_ptr` with a custom deleter (`bson_free`), ensuring that there are
 * no memory leaks even in the case of exceptions.
 *
 * @param doc The `aevum::bson::doc::Document` to be serialized.
 * @return A `std::string` containing the JSON representation of the document. If the
 *         input document is empty or if the serialization process fails, an empty
 *         JSON object `"{}"` is returned as a safe fallback.
 */
std::string to_string(const aevum::bson::doc::Document &doc) {
    if (doc.empty() || !doc.get()) {
        return "{}";
    }

    size_t len;
    // Use a smart pointer with a custom deleter to ensure the C-style string from bson_as_json is
    // always freed.
    std::unique_ptr<char, decltype(&bson_free)> json_str(
        bson_as_relaxed_extended_json(doc.get(), &len), &bson_free);

    if (!json_str) {
        aevum::util::log::Logger::error(
            "BSON-JSON-Serializer: Failed to serialize BSON Document to JSON string. The operation "
            "returned a null pointer.");
        return "{}";  // Return an empty JSON object on failure.
    }

    return {json_str.get(), len};
}

}  // namespace aevum::bson::json

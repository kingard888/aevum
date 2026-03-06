// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file validator.cpp
 * @brief Implements the C++-to-Rust bridge for BSON schema validation.
 * @details This translation unit contains the implementation of the `validate_via_rust` function.
 * It is responsible for marshalling BSON data into a JSON string format suitable for the
 * Foreign Function Interface (FFI) and interpreting the result from the Rust core.
 */
#include "aevum/db/schema/validator.hpp"

#include "aevum/bson/json/serializer.hpp"
#include "aevum/db/ffi.hpp"  // For the rust_validate FFI function.

namespace aevum::db::schema {

/**
 * @brief Executes schema validation by serializing BSON to JSON and calling the Rust FFI.
 * @details This function orchestrates the cross-language validation process.
 *
 * 1.  **Precondition Check**: It first validates that both the document and the schema are
 * non-empty. Attempting to validate against an empty schema or an empty document is considered an
 * invalid operation, so it returns `false` immediately.
 * 2.  **Data Marshalling**: It serializes the `doc` and `schema` BSON objects into their respective
 *     canonical JSON string representations using the `aevum::bson::json::to_string` utility.
 * 3.  **FFI Dispatch**: The resulting JSON C-style strings are passed as arguments to the
 *     `rust_validate` function, which is imported from the Rust FFI layer.
 * 4.  **Result Interpretation**: The integer result from the FFI (`1` for true, `0` for false) is
 *     converted into a boolean value and returned to the C++ caller.
 *
 * This entire process is marked `noexcept`, as any potential exceptions during JSON serialization
 * or other operations are handled internally, resulting in a boolean `false` rather than an
 * exception propagation.
 *
 * @param doc The `aevum::bson::doc::Document` to be validated.
 * @param schema The `aevum::bson::doc::Document` representing the validation rules.
 * @return `true` if the `rust_validate` FFI function returns `1`, indicating that the document
 *         conforms to the schema. Returns `false` in all other cases, including validation
 *         failure, empty inputs, or internal serialization errors.
 */
bool validate_via_rust(const aevum::bson::doc::Document &doc,
                       const aevum::bson::doc::Document &schema) noexcept {
    // A document cannot be valid against an empty schema, nor can an empty document be valid.
    if (doc.empty() || schema.empty()) {
        return false;
    }

    // Convert the BSON document and schema into JSON strings, as required by the Rust FFI.
    // This step marshals the data into a format that is universally understood across the FFI
    // boundary.
    std::string doc_json = aevum::bson::json::to_string(doc);
    std::string schema_json = aevum::bson::json::to_string(schema);

    // Dispatch to the Rust core for the actual validation logic and interpret the integer result.
    return rust_validate(doc_json.c_str(), schema_json.c_str()) == 1;
}

}  // namespace aevum::db::schema

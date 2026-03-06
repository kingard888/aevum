// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file validator.hpp
 * @brief Declares the bridge function for dispatching BSON schema validation tasks to the
 * high-performance Rust core.
 * @details This header provides the interface for `validate_via_rust`, a critical function that
 * acts as the conduit between the C++ BSON representation and the Rust-based validation engine.
 * It is designed as a stateless, `noexcept` utility.
 */
#pragma once

#include "aevum/bson/doc/document.hpp"

namespace aevum::db::schema {

/**
 * @brief Validates a BSON document against a BSON schema by leveraging the Rust FFI validation
 * engine.
 * @details This function serves as the primary C++-to-Rust bridge for schema validation. Its core
 * responsibility is to perform the necessary data transformation, converting the native BSON
 * `Document` objects into their canonical JSON string representations. These JSON strings are then
 * passed across the FFI boundary to the `rust_validate` function, which executes the validation
 * logic. This design isolates the high-performance validation logic within the Rust crate while
 * providing a seamless integration point for the C++ application layer.
 *
 * @param doc A constant reference to the `aevum::bson::doc::Document` to be validated.
 * @param schema A constant reference to the `aevum::bson::doc::Document` that defines the schema
 * rules.
 * @return `true` if the document fully conforms to the specified schema.
 * @return `false` if the document violates any schema rules, or if either the document or schema
 *         is empty or cannot be serialized to JSON.
 */
[[nodiscard]] bool validate_via_rust(const aevum::bson::doc::Document &doc,
                                     const aevum::bson::doc::Document &schema) noexcept;

}  // namespace aevum::db::schema

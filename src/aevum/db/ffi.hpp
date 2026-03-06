// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file ffi.hpp
 * @brief Declares the Foreign Function Interface (FFI) for interacting with the Rust-native query
 * engine.
 * @details This header provides the C++ declarations for the functions and structures exposed by
 * the `aevum_ffi` Rust crate. It defines the low-level bridge through which the C++ core dispatches
 * document matching, schema validation, and complex query operations to the high-performance Rust
 * implementation.
 */
#pragma once

#include <cstdint>

extern "C" {

/**
 * @struct rust_update_result
 * @brief Represents the data returned by the `rust_update` function.
 */
struct rust_update_result {
    /** @brief A pointer to a null-terminated JSON string containing the modified dataset. Owned by
     * Rust. */
    char *data;
    /** @brief The number of documents that were actually updated. */
    int32_t modified_count;
};

/**
 * @brief Validates a JSON document against a JSON schema query.
 * @param doc A null-terminated JSON string of the document.
 * @param schema A null-terminated JSON string of the schema query.
 * @return 1 if valid, 0 otherwise.
 */
int32_t rust_validate(const char *doc, const char *schema);

/**
 * @brief Counts the documents in a JSON array that match a query.
 * @param data A null-terminated JSON string of an array of documents.
 * @param query A null-terminated JSON string of the query object.
 * @return The number of matching documents.
 */
int32_t rust_count(const char *data, const char *query);

/**
 * @brief Executes a comprehensive find operation on a dataset.
 * @return A pointer to a heap-allocated JSON string array of results. MUST be freed via
 * rust_free_string.
 */
char *rust_find(const char *data, const char *query, const char *sort, const char *projection,
                int32_t limit, int32_t skip);

/**
 * @brief Executes an update operation on a dataset, respecting an optional schema.
 * @param data A JSON array of documents.
 * @param query The query to select documents for update.
 * @param update_doc The update specification.
 * @param schema The schema to validate against (can be empty string).
 * @return A `rust_update_result` containing the new data and modified count.
 */
rust_update_result rust_update(const char *data, const char *query, const char *update_doc,
                               const char *schema);

/**
 * @brief Deletes matching documents from a dataset.
 * @return A pointer to the modified JSON array. MUST be freed via rust_free_string.
 */
char *rust_delete(const char *data, const char *query);

/**
 * @brief Deallocates a string that was allocated by the Rust memory manager.
 * @param s The pointer to the string to free.
 */
void rust_free_string(char *s);

/**
 * @brief Deallocates the resources within a `rust_update_result`.
 * @param res The result struct to free.
 */
void rust_free_update_result(rust_update_result res);

}  // extern "C"

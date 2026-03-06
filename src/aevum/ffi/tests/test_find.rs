// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

//! # Integration Tests for Document Retrieval (Find)
//!
//! This test suite focuses on the `rust_find` FFI function, verifying that AevumDB's sophisticated
//! data retrieval engine is correctly exposed. It exercises the complex interaction between
//! filtering, sorting, field projection, and pagination (skip/limit) across the FFI boundary.

mod common;

use aevum_ffi::{rust_find, rust_free_string};
use common::{from_c_char_ptr, to_c_char_ptr};
use libc::c_char;
use serde_json::{json, Value};

#[test]
/// Exercises the `rust_find` function across multiple complex querying scenarios.
///
/// This test verifies:
/// 1.  **Basic Retrieval**: Fetching a subset of documents with a simple limit.
/// 2.  **Advanced Querying**: Combining filtering, descending sort, and field projection.
/// 3.  **Pagination and Ordering**: Ensuring that `skip` and `limit` correctly interact with
///     an ascending sort to provide deterministic results.
/// 4.  **Error Handling**: Verifying that malformed JSON dataset inputs result in an empty array
///     response rather than a system failure.
fn test_ffi_document_retrieval() {
    let dataset = r#"[
        { "name": "Alice", "age": 30, "city": "New York", "_id": "A1" },
        { "name": "Bob", "age": 25, "city": "London", "_id": "B2" },
        { "name": "Charlie", "age": 35, "city": "New York", "_id": "C3" },
        { "name": "David", "age": 25, "city": "London", "_id": "D4" }
    ]"#;
    let c_data = to_c_char_ptr(dataset);

    // Scenario 1: Retrieve all documents, but limit the result set to 2.
    let result1_ptr = unsafe {
        rust_find(c_data, to_c_char_ptr("{}"), to_c_char_ptr("{}"), to_c_char_ptr("{}"), 2, 0)
    };
    let result1_str = unsafe { from_c_char_ptr(result1_ptr) };
    let expected1 = json!([
        { "name": "Alice", "age": 30, "city": "New York", "_id": "A1" },
        { "name": "Bob", "age": 25, "city": "London", "_id": "B2" }
    ]);
    assert_eq!(serde_json::from_str::<Value>(&result1_str).unwrap(), expected1);

    // Scenario 2: Filter by city, sort by age descending, and project only specific fields.
    let result2_ptr = unsafe {
        rust_find(
            c_data,
            to_c_char_ptr(r#"{ "city": "New York" }"#),
            to_c_char_ptr(r#"{ "age": -1 }"#),
            to_c_char_ptr(r#"{ "name": 1, "age": 1, "_id": 0 }"#),
            0,
            0,
        )
    };
    let result2_str = unsafe { from_c_char_ptr(result2_ptr) };
    let expected2 = json!([
        { "name": "Charlie", "age": 35 },
        { "name": "Alice", "age": 30 }
    ]);
    assert_eq!(serde_json::from_str::<Value>(&result2_str).unwrap(), expected2);

    // Scenario 3: Complex pagination. Skip the first 2 documents and take the next 1, sorted by age.
    // Full sorted order: Bob(25), David(25), Alice(30), Charlie(35).
    // Skipping 2 (Bob, David) and taking 1 should return Alice.
    let result3_ptr = unsafe {
        rust_find(
            c_data,
            to_c_char_ptr("{}"),
            to_c_char_ptr(r#"{ "age": 1 }"#),
            to_c_char_ptr("{}"),
            1,
            2,
        )
    };
    let result3_str = unsafe { from_c_char_ptr(result3_ptr) };
    let expected3 = json!([{ "name": "Alice", "age": 30, "city": "New York", "_id": "A1" }]);
    assert_eq!(serde_json::from_str::<Value>(&result3_str).unwrap(), expected3);

    // Scenario 4: Handling of a syntactically invalid JSON dataset.
    let c_malformed_data = to_c_char_ptr(r#"[ { "name": "Broken" ]"#);
    let result4_ptr = unsafe {
        rust_find(
            c_malformed_data,
            to_c_char_ptr("{}"),
            to_c_char_ptr("{}"),
            to_c_char_ptr("{}"),
            0,
            0,
        )
    };
    let result4_str = unsafe { from_c_char_ptr(result4_ptr) };
    assert_eq!(
        result4_str, "[]",
        "The engine must return an empty JSON array for malformed input."
    );

    unsafe {
        rust_free_string(c_data as *mut c_char);
        rust_free_string(c_malformed_data as *mut c_char);
    }
}

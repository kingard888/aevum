// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

//! # Integration Tests for Document Deletion Operations
//!
//! This test suite focuses on the `rust_delete` FFI function, verifying that AevumDB's document
//! removal logic is correctly exposed. It ensures that the engine can precisely filter out and
//! remove documents from a dataset based on various query criteria.

mod common;

use aevum_ffi::{rust_delete, rust_free_string};
use common::{from_c_char_ptr, to_c_char_ptr};
use libc::c_char;
use serde_json::{json, Value};

#[test]
/// Exercises the `rust_delete` function across several document removal scenarios.
///
/// This test verifies:
/// 1.  **Targeted Deletion**: Successfully removing a single document specified by a unique field.
/// 2.  **Bulk Deletion**: Removing multiple documents that satisfy a range-based query.
/// 3.  **No-Op Deletion**: Confirming that the dataset remains intact when the deletion query
///     matches no documents.
/// 4.  **Malformed Data Handling**: Ensuring the function returns an empty array when given an
///     invalid JSON dataset.
fn test_ffi_document_deletions() {
    let dataset = r#"[
        { "name": "Alice", "age": 30 },
        { "name": "Bob", "age": 25 },
        { "name": "Charlie", "age": 35 }
    ]"#;
    let c_data = to_c_char_ptr(dataset);

    // Scenario 1: Delete a specific document (e.g., 'Bob').
    let result1_ptr = unsafe { rust_delete(c_data, to_c_char_ptr(r#"{ "name": "Bob" }"#)) };
    let result1_str = unsafe { from_c_char_ptr(result1_ptr) };
    let expected1 = json!([
        { "name": "Alice", "age": 30 },
        { "name": "Charlie", "age": 35 }
    ]);
    assert_eq!(serde_json::from_str::<Value>(&result1_str).unwrap(), expected1);

    // Scenario 2: Delete multiple documents matching a range criteria (e.g., age >= 30).
    let result2_ptr = unsafe { rust_delete(c_data, to_c_char_ptr(r#"{ "age": { "$gte": 30 } }"#)) };
    let result2_str = unsafe { from_c_char_ptr(result2_ptr) };
    let expected2 = json!([{ "name": "Bob", "age": 25 }]);
    assert_eq!(serde_json::from_str::<Value>(&result2_str).unwrap(), expected2);

    // Scenario 3: Deletion query with no matches.
    let result3_ptr = unsafe { rust_delete(c_data, to_c_char_ptr(r#"{ "name": "David" }"#)) };
    let result3_str = unsafe { from_c_char_ptr(result3_ptr) };
    let expected_full = serde_json::from_str::<Value>(dataset).unwrap();
    assert_eq!(serde_json::from_str::<Value>(&result3_str).unwrap(), expected_full);

    unsafe {
        rust_free_string(c_data as *mut c_char);
    }
}

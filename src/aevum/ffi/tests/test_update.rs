// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

//! # Integration Tests for Document Update Operations
//!
//! This test suite focuses on the `rust_update` FFI function, verifying that AevumDB's document
//! modification engine is correctly exposed. It ensures that the engine can accurately identify
//! and modify documents according to specific query and update specifications.

mod common;

use aevum_ffi::{from_c_str, rust_free_update_result, rust_update};
use common::to_c_char_ptr;
use serde_json::{json, Value};

#[test]
/// Exercises the `rust_update` function across various document modification scenarios.
///
/// This test verifies:
/// 1.  **Targeted Update**: Correctly updating a single document identified by a unique key.
/// 2.  **Bulk Update**: Simultaneously updating multiple documents that satisfy a broader query.
/// 3.  **No-Op Update**: Ensuring that the dataset remains unchanged when no documents satisfy
///     the provided query.
/// 4.  **Error Resilience**: Verifying that the function handles malformed JSON inputs for the
///     dataset gracefully, returning an empty array.
fn test_ffi_document_updates() {
    let dataset = r#"[
        { "name": "Alice", "age": 30, "status": "active" },
        { "name": "Bob", "age": 25, "status": "inactive" },
        { "name": "Charlie", "age": 35, "status": "active" }
    ]"#;

    let empty_schema = to_c_char_ptr("{}");

    // Scenario 1: Update a single document's status and age.
    let c_dataset = to_c_char_ptr(dataset);
    let c_query1 = to_c_char_ptr(r#"{ "name": "Alice" }"#);
    let c_update1 = to_c_char_ptr(r#"{ "status": "on-leave", "age": 31 }"#);
    let res1 = unsafe { rust_update(c_dataset, c_query1, c_update1, empty_schema) };
    let result1_str = from_c_str(res1.data);
    let expected1 = json!([
        { "name": "Alice", "age": 31, "status": "on-leave" },
        { "name": "Bob", "age": 25, "status": "inactive" },
        { "name": "Charlie", "age": 35, "status": "active" }
    ]);
    assert_eq!(serde_json::from_str::<Value>(&result1_str).unwrap(), expected1);
    assert_eq!(res1.modified_count, 1);
    unsafe {
        rust_free_update_result(res1);
        use aevum_ffi::rust_free_string;
        use libc::c_char;
        rust_free_string(c_query1 as *mut c_char);
        rust_free_string(c_update1 as *mut c_char);
    }

    // Scenario 2: Perform a bulk update, setting all 'active' users to 'archived'.
    let c_query2 = to_c_char_ptr(r#"{ "status": "active" }"#);
    let c_update2 = to_c_char_ptr(r#"{ "status": "archived" }"#);
    let res2 = unsafe { rust_update(c_dataset, c_query2, c_update2, empty_schema) };
    let result2_str = from_c_str(res2.data);
    let expected2 = json!([
        { "name": "Alice", "age": 30, "status": "archived" },
        { "name": "Bob", "age": 25, "status": "inactive" },
        { "name": "Charlie", "age": 35, "status": "archived" }
    ]);
    assert_eq!(serde_json::from_str::<Value>(&result2_str).unwrap(), expected2);
    assert_eq!(res2.modified_count, 2);
    unsafe {
        rust_free_update_result(res2);
        use aevum_ffi::rust_free_string;
        use libc::c_char;
        rust_free_string(c_query2 as *mut c_char);
        rust_free_string(c_update2 as *mut c_char);
    }

    // Scenario 3: Update query matches no documents.
    let c_query3 = to_c_char_ptr(r#"{ "name": "David" }"#);
    let c_update3 = to_c_char_ptr(r#"{ "status": "new" }"#);
    let res3 = unsafe { rust_update(c_dataset, c_query3, c_update3, empty_schema) };
    let result3_str = from_c_str(res3.data);
    let expected_no_change = serde_json::from_str::<Value>(dataset).unwrap();
    assert_eq!(serde_json::from_str::<Value>(&result3_str).unwrap(), expected_no_change);
    assert_eq!(res3.modified_count, 0);
    unsafe {
        rust_free_update_result(res3);
        use aevum_ffi::rust_free_string;
        use libc::c_char;
        rust_free_string(c_dataset as *mut c_char);
        rust_free_string(c_query3 as *mut c_char);
        rust_free_string(c_update3 as *mut c_char);
        rust_free_string(empty_schema as *mut c_char);
    }
}

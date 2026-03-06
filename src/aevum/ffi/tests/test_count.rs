// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

//! # Integration Tests for Document Counting Operations
//!
//! This test suite focuses on the `rust_count` FFI function, verifying that AevumDB's high-performance
//! document counting logic is correctly exposed. It ensures that the parallelized counting
//! engine accurately filters and aggregates documents according to complex query criteria.

mod common;

use aevum_ffi::{rust_count, rust_free_string};
use common::to_c_char_ptr;
use libc::c_char;

#[test]
/// Exercises the `rust_count` function across a variety of dataset and query scenarios.
///
/// This test verifies:
/// 1.  **Full Dataset Scan**: Correctly counting all documents when an empty query is provided.
/// 2.  **Targeted Filtering**: Accurately counting documents that match specific field values.
/// 3.  **Range-Based Queries**: Evaluating queries that utilize relational operators like `$gt`.
/// 4.  **Empty Result Sets**: Ensuring the function returns 0 when no documents satisfy the criteria.
/// 5.  **Robust Error Ingestion**: Verifying that malformed JSON data in the dataset is handled
///     without crashing, returning a zero count.
fn test_ffi_document_counting() {
    let dataset = r#"[
        { "id": 1, "status": "active", "value": 10 },
        { "id": 2, "status": "inactive", "value": 20 },
        { "id": 3, "status": "active", "value": 15 },
        { "id": 4, "status": "pending", "value": 30 }
    ]"#;
    let c_data = to_c_char_ptr(dataset);

    // Scenario 1: Count every document in the array.
    let c_query_all = to_c_char_ptr("{}");
    let count_all = unsafe { rust_count(c_data, c_query_all) };
    assert_eq!(count_all, 4, "The engine should have counted every document in the dataset.");
    unsafe {
        rust_free_string(c_query_all as *mut c_char);
    }

    // Scenario 2: Count only documents marked as 'active'.
    let c_query_active = to_c_char_ptr(r#"{ "status": "active" }"#);
    let count_active = unsafe { rust_count(c_data, c_query_active) };
    assert_eq!(
        count_active, 2,
        "The engine failed to accurately filter by a specific field value."
    );
    unsafe {
        rust_free_string(c_query_active as *mut c_char);
    }

    // Scenario 3: Count documents with a 'value' greater than 12.
    let c_query_gt_12 = to_c_char_ptr(r#"{ "value": { "$gt": 12 } }"#);
    let count_gt_12 = unsafe { rust_count(c_data, c_query_gt_12) };
    assert_eq!(count_gt_12, 3, "The engine failed to evaluate a relational operator ($gt) query.");
    unsafe {
        rust_free_string(c_query_gt_12 as *mut c_char);
    }

    // Scenario 4: Query that matches no documents.
    let c_query_none = to_c_char_ptr(r#"{ "status": "archived" }"#);
    let count_none = unsafe { rust_count(c_data, c_query_none) };
    assert_eq!(count_none, 0, "The engine should have returned zero for a query with no matches.");
    unsafe {
        rust_free_string(c_query_none as *mut c_char);
    }

    // Scenario 5: Dataset contains malformed JSON syntax.
    let malformed_dataset = r#"[ { "id": 1, ]"#;
    let c_malformed_data = to_c_char_ptr(malformed_dataset);
    let c_wildcard_query = to_c_char_ptr("{}");
    let count_malformed = unsafe { rust_count(c_malformed_data, c_wildcard_query) };
    assert_eq!(
        count_malformed, 0,
        "The engine must return 0 when encountering malformed input data."
    );

    unsafe {
        rust_free_string(c_data as *mut c_char);
        rust_free_string(c_malformed_data as *mut c_char);
        rust_free_string(c_wildcard_query as *mut c_char);
    }
}

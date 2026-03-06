// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

//! # Integration Tests for Document Schema Validation
//!
//! This test suite focuses on the `rust_validate` FFI function, verifying that AevumDB's schema
//! validation logic is correctly exposed and accessible to C/C++ clients. It rigorously tests the
//! boundary between the JSON text representation and the internal validation engine.

mod common;

use aevum_ffi::{rust_free_string, rust_validate};
use common::to_c_char_ptr;
use libc::c_char;

#[test]
/// Exercises the `rust_validate` function across a diverse set of validation scenarios.
///
/// This test verifies that the FFI correctly handles:
/// 1.  **Positive Validation**: A syntactically and semantically valid document that adheres
///     to a comprehensive schema.
/// 2.  **Negative Validation**: A document that is syntactically valid JSON but fails to
///     meet the criteria defined by the schema.
/// 3.  **Malformed JSON Ingestion**: Ensuring that malformed JSON input for either the
///     document or the schema is handled gracefully, resulting in a validation failure
///     rather than a system crash.
fn test_ffi_document_validation() {
    // Scenario 1: A valid document should pass validation against a matching schema.
    let doc_str = r#"{ "name": "Alice", "age": 30, "metadata": { "active": true } }"#;
    let schema_str = r#"{ "name": { "$eq": "Alice" }, "age": { "$gt": 20 } }"#;

    let c_doc = to_c_char_ptr(doc_str);
    let c_schema = to_c_char_ptr(schema_str);

    let is_valid = unsafe { rust_validate(c_doc, c_schema) };
    assert_eq!(is_valid, 1, "Validation should have succeeded for a matching document.");

    // Critical: Memory allocated for input pointers must be freed.
    unsafe {
        rust_free_string(c_doc as *mut c_char);
        rust_free_string(c_schema as *mut c_char);
    }

    // Scenario 2: An invalid document (e.g., under the age limit) must fail validation.
    let invalid_doc_str = r#"{ "name": "Bob", "age": 15 }"#;
    let c_invalid_doc = to_c_char_ptr(invalid_doc_str);
    let c_matching_schema = to_c_char_ptr(schema_str);

    let is_invalid = unsafe { rust_validate(c_invalid_doc, c_matching_schema) };
    assert_eq!(is_invalid, 0, "Validation should have failed for a non-matching document.");

    unsafe {
        rust_free_string(c_invalid_doc as *mut c_char);
        rust_free_string(c_matching_schema as *mut c_char);
    }

    // Scenario 3: Malformed JSON syntax in the document input.
    let malformed_doc = r#"{ "incomplete": }"#;
    let c_malformed = to_c_char_ptr(malformed_doc);
    let c_empty_schema = to_c_char_ptr("{}");

    let is_malformed_rejected = unsafe { rust_validate(c_malformed, c_empty_schema) };
    assert_eq!(is_malformed_rejected, 0, "A malformed JSON document must be rejected.");

    unsafe {
        rust_free_string(c_malformed as *mut c_char);
        rust_free_string(c_empty_schema as *mut c_char);
    }
}

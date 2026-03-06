// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

#![doc(
    html_root_url = "https://docs.rs/aevum_ffi/0.1.0",
    html_logo_url = "https://www.aevumdb.com/logo.png"
)]
#![warn(missing_docs, unreachable_pub, unstable_features, unused_must_use, unused_mut)]
#![deny(unsafe_op_in_unsafe_fn, unused_unsafe, unconditional_recursion, clippy::all)]

//! # The AevumDB Foreign Function Interface (FFI) Crate
//!
//! `aevum_ffi` serves as the primary C-compatible bridge to the high-performance, Rust-native
//! AevumDB query engine. This crate is meticulously engineered to provide a safe, ergonomic, and
//! efficient interoperability layer, enabling external C and C++ applications to leverage the
//! full power of AevumDB's document querying and manipulation capabilities.
//!
//! The library exposes a suite of `extern "C"` functions that encapsulate critical database
//! operations, from schema validation and document counting to complex data retrieval and
//! modification. It also furnishes a robust set of utilities for managing the impedance mismatch
//! between Rust's ownership model and C's manual memory management, particularly concerning
//! string data.
//!
//! ## Core Architectural Pillars
//!
//! - **FFI Utilities**: A collection of functions dedicated to the safe marshalling of data types
//!   across the FFI boundary. This includes converting C-style strings to Rust's native `String`
//!   and vice-versa, as well as providing a mechanism for C/C++ callers to correctly deallocate
//!   memory that was allocated by Rust.
//! - **Query Operations**: A comprehensive set of functions that expose AevumDB's core document
//!   processing logic, including `find`, `count`, `update`, `delete`, and `validate`.
//!
//! ## Critical Safety Contract for C/C++ Callers
//!
//! Every function exposed through this FFI layer and marked as `extern "C"` is inherently `unsafe`
//! from the perspective of the Rust compiler. The C/C++ caller is therefore contractually obligated
//! to uphold a set of invariants to prevent undefined behavior. Violation of these rules can lead
//! to severe issues, including memory corruption, security vulnerabilities, and application crashes.
//!
//! ### Pointer Validity
//! All `*const c_char` pointer arguments (such as `doc`, `query`, `schema`, etc.) **must** point to
//! valid, non-null, and properly null-terminated UTF-8 encoded C-style strings. Furthermore, the
//! memory these pointers reference **must** remain valid and accessible for the entire duration of
//! the function call. Passing dangling pointers, uninitialized pointers, or non-null-terminated
//! data will result in undefined behavior.
//!
//! ### Memory Management and Ownership
//! Any memory returned by this library in the form of a `*mut c_char` pointer (e.g., the result of
//! `rust_find`) has been allocated by Rust's memory allocator. Consequently, ownership of this memory
//! is transferred to the caller. The C/C++ caller **must** take responsibility for deallocating this
//! memory by calling the provided `rust_free_string` function. Failure to do so will result in a
//! memory leak.
//!
//! ### Pointer Integrity for Deallocation
//! Pointers passed to `rust_free_string` **must** be the exact, unmodified pointers that were originally
//! returned by this FFI layer. The pointer must not have been previously freed. Attempting to free a
//! pointer twice (a double-free) or a pointer not allocated by this library will lead to heap
//! corruption and undefined behavior.

use libc::{c_char, c_int};

/// Provides foundational utilities for the FFI boundary, including robust string conversions
/// and a mechanism for C-side memory deallocation of Rust-allocated strings.
pub mod util;

/// Encapsulates the complete, self-contained query processing engine, including modules for
/// document matching, operator evaluation, sorting, and field projection.
pub mod query;

// Re-export public FFI functions and modules to create a flattened, more accessible API surface.
// This design choice simplifies linking and usage from external C/C++ code, as consumers
// do not need to be aware of the internal module structure.
pub use crate::query::operations::{count, delete_docs, find, update, validate};
pub use query::*;
pub use util::*;

/// Represents the result of a database update operation returned through the FFI.
#[repr(C)]
pub struct rust_update_result {
    /// A pointer to the JSON string containing the modified dataset.
    pub data: *mut c_char,
    /// The number of documents that were successfully updated.
    pub modified_count: c_int,
}

/// FFI-exposed function to validate a JSON document against a specified JSON schema query.
///
/// This function serves as the C-compatible entry point for AevumDB's schema validation logic. It
/// expects both the document and the schema to be provided as valid, null-terminated JSON strings.
///
/// # Arguments
///
/// * `doc` - A raw pointer to a constant, null-terminated, UTF-8 encoded C-style string
///   representing the JSON document to be validated.
/// * `schema` - A raw pointer to a constant, null-terminated, UTF-8 encoded C-style string
///   representing the JSON schema query to validate against.
///
/// # Returns
///
/// * `1` (`c_int`) if the document successfully validates against the schema.
/// * `0` (`c_int`) if the document is invalid, or if a processing error occurs (e.g., either
///   input string is malformed JSON).
///
/// # Safety
///
/// This function is `unsafe` and imposes a strict contract on the caller. The caller **must** ensure
/// that both `doc` and `schema` are valid, non-null pointers to null-terminated C-style strings,
/// and that the memory they point to remains valid for the entire duration of this function call.
/// Passing invalid pointers will invoke undefined behavior.
///
/// # Example (from a C perspective)
///
/// ```c
/// #include "aevum_ffi.h"
/// #include <assert.h>
///
/// const char* doc = "{ \"name\": \"Alice\", \"age\": 30 }";
/// const char* schema = "{ \"age\": { \"$gt\": 25 } }";
///
/// int is_valid = rust_validate(doc, schema);
/// assert(is_valid == 1); // This should succeed.
/// ```
#[no_mangle]
pub unsafe extern "C" fn rust_validate(doc: *const c_char, schema: *const c_char) -> c_int {
    let doc_str = crate::from_c_str(doc);
    let schema_str = crate::from_c_str(schema);

    if validate(&doc_str, &schema_str) {
        1
    } else {
        0
    }
}

/// FFI-exposed function to count the number of documents in a dataset that match a given query.
///
/// # Arguments
///
/// * `data` - A pointer to a null-terminated, UTF-8 encoded C string representing the dataset,
///   which must be a valid JSON array of documents.
/// * `query` - A pointer to a null-terminated, UTF-8 encoded C string representing the query,
///   which must be a valid JSON object. An empty object (`{}`) will match all documents.
///
/// # Returns
///
/// A `c_int` representing the total number of matching documents. Returns `0` if an error occurs,
/// such as if the input strings are not valid JSON, or if no documents match the query.
///
/// # Safety
///
/// The caller **must** ensure that both `data` and `query` are valid, non-null pointers to
/// null-terminated C-style strings that remain valid for the duration of this call.
#[no_mangle]
pub unsafe extern "C" fn rust_count(data: *const c_char, query: *const c_char) -> c_int {
    let data_str = crate::from_c_str(data);
    let query_str = crate::from_c_str(query);
    count(&data_str, &query_str) as c_int
}

/// FFI-exposed function to find and retrieve documents from a dataset based on a comprehensive
/// set of query, sort, and projection criteria.
///
/// # Arguments
///
/// * `data` - A C string representing the JSON array of documents to query.
/// * `query` - A C string representing the JSON object that defines the filtering conditions.
/// * `sort` - A C string representing the JSON object that defines the sort order
///   (e.g., `{"field": 1}` for ascending, `{"field": -1}` for descending).
/// * `projection` - A C string representing the JSON object that defines which document fields
///   to include or exclude in the result set.
/// * `limit` - A `c_int` specifying the maximum number of documents to return. A value of `0`
///   signifies no limit.
/// * `skip` - A `c_int` specifying the number of documents to skip at the beginning of the
///   result set, for pagination.
///
/// # Returns
///
/// A `*mut c_char` pointer to a new, heap-allocated, null-terminated UTF-8 string. This string
/// contains the query results formatted as a JSON array.
///
/// **Crucially, this returned pointer's memory is owned by the caller** and **must** be deallocated
/// by passing it to `rust_free_string`. Returns a null pointer (`ptr::null_mut()`) if a fatal
/// error occurs, such as an allocation failure or if the input strings are malformed.
///
/// # Safety
///
/// The caller is responsible for upholding the standard pointer validity contract for all input
/// C strings. The caller also assumes ownership of the returned pointer and is responsible for its
/// deallocation via `rust_free_string`.
#[no_mangle]
pub unsafe extern "C" fn rust_find(
    data: *const c_char,
    query: *const c_char,
    sort: *const c_char,
    projection: *const c_char,
    limit: c_int,
    skip: c_int,
) -> *mut c_char {
    let l = if limit < 0 { 0 } else { limit as usize };
    let s = if skip < 0 { 0 } else { skip as usize };

    let result_string = find(
        &crate::from_c_str(data),
        &crate::from_c_str(query),
        &crate::from_c_str(sort),
        &crate::from_c_str(projection),
        l,
        s,
    );

    crate::to_c_string(result_string)
}

/// FFI-exposed function to find documents matching a query and update them according to an
/// update document, while respecting a schema.
///
/// # Arguments
///
/// * `data` - A C string representing the JSON array of documents to be modified.
/// * `query` - A C string representing the JSON object query to select which documents to update.
/// * `update_doc_str` - A C string representing the JSON object that defines the fields to set or
///   overwrite in the matched documents.
/// * `schema_str` - A C string representing the JSON schema to validate updated documents against.
///
/// # Returns
///
/// A `rust_update_result` struct containing the new data and modified count.
///
/// # Safety
///
/// The caller must ensure all input C strings are valid for the duration of the call and must
/// deallocate the returned data pointer using `rust_free_string` or use `rust_free_update_result`.
#[no_mangle]
pub unsafe extern "C" fn rust_update(
    data: *const c_char,
    query: *const c_char,
    update_doc_str: *const c_char,
    schema_str: *const c_char,
) -> rust_update_result {
    let (result_string, count) = update(
        &crate::from_c_str(data),
        &crate::from_c_str(query),
        &crate::from_c_str(update_doc_str),
        &crate::from_c_str(schema_str),
    );
    rust_update_result { data: crate::to_c_string(result_string), modified_count: count as c_int }
}

/// Frees a `rust_update_result` struct and its internal data.
///
/// # Safety
/// The provided pointer must be a valid, non-null pointer to a `rust_update_result` allocated by
/// Rust.
#[no_mangle]
pub unsafe extern "C" fn rust_free_update_result(res: rust_update_result) {
    if !res.data.is_null() {
        unsafe {
            crate::rust_free_string(res.data);
        }
    }
}

/// FFI-exposed function to delete documents from a dataset that match a given query.
///
/// # Arguments
///
/// * `data` - A C string representing the JSON array of documents to be modified.
/// * `query` - A C string representing the JSON object query to select which documents to delete.
///
/// # Returns
///
/// A `*mut c_char` pointer to a new string containing the dataset after the deletions have been
/// performed. This pointer's memory is owned by the caller and **must** be freed using
/// `rust_free_string`. Returns a null pointer on error.
///
/// # Safety
///
/// The caller must ensure all input C strings are valid for the duration of the call and must
/// deallocate the returned pointer using `rust_free_string`.
#[no_mangle]
pub unsafe extern "C" fn rust_delete(data: *const c_char, query: *const c_char) -> *mut c_char {
    let result_string = delete_docs(&crate::from_c_str(data), &crate::from_c_str(query));
    crate::to_c_string(result_string)
}

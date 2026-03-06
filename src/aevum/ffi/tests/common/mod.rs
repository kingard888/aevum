// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

//! # Common Utilities for FFI Integration Testing
//!
//! This module provides a set of shared, internal helper functions and abstractions designed to
//! facilitate the testing of the Foreign Function Interface (FFI) layer. These utilities
//! encapsulate the boilerplate code required for data marshalling and memory management,
//! allowing the integration tests to focus on the semantic correctness of the FFI operations.
//!
//! The helpers provided here are specifically tailored to bridge the gap between Rust's high-level
//! string types and the raw C-style pointers expected and returned by the `extern "C"` functions.

use aevum_ffi::{from_c_str, rust_free_string, to_c_string};
use libc::c_char;

/// A test-only utility function that transforms a Rust string slice into an FFI-compatible,
/// heap-allocated, and null-terminated C-string pointer.
///
/// This function acts as a bridge for providing input data to the `rust_*` functions under test.
/// It creates a temporary owned `String` and then leverages the library's `to_c_string` utility
/// to perform the actual allocation and conversion.
///
/// # Arguments
///
/// * `s` - A string slice (`&str`) representing the data to be marshalled to C.
///
/// # Returns
///
/// A `*const c_char` pointer to the newly allocated C-string. Note that while this is returned
/// as a constant pointer for input compatibility, the underlying memory is heap-allocated by Rust.
pub fn to_c_char_ptr(s: &str) -> *const c_char {
    to_c_string(s.to_string())
}

/// A test-only utility function that consumes a raw C-string pointer returned from the FFI layer,
/// converts its contents back into an owned Rust `String`, and then rigorously deallocates the
/// C-string's memory to ensure a leak-free test execution.
///
/// This helper is crucial for simulating the responsibilities of an external C/C++ caller. It
/// completes the FFI memory management contract by returning ownership of the pointer to the
/// Rust allocator via `rust_free_string`.
///
/// # Arguments
///
/// * `ptr` - A `*mut c_char` pointer returned by an FFI function, representing memory that was
///   allocated on the Rust heap and transferred to the caller.
///
/// # Returns
///
/// An owned Rust `String` containing the data that was stored at the pointer's location.
///
/// # Safety
///
/// This function is marked `unsafe` because it invokes `rust_free_string` on a raw pointer,
/// which involves manual memory deallocation. It relies on the pointer being valid and
/// originally allocated by the AevumDB FFI layer.
pub unsafe fn from_c_char_ptr(ptr: *mut c_char) -> String {
    let s = from_c_str(ptr);
    // The following call is the primary mechanism for preventing memory leaks during testing.
    // It returns the pointer to the Rust allocator for proper destruction.
    unsafe { rust_free_string(ptr) };
    s
}

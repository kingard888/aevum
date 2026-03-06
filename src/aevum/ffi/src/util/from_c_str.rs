// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

use libc::c_char;
use std::ffi::CStr;

/// Safely converts a null-terminated C-style string (`*const c_char`) into an owned Rust `String`.
///
/// This function is a critical utility for ingesting string data from a C/C++ caller into the
/// Rust environment. It is designed with safety and robustness as paramount concerns, gracefully
/// handling potential null pointers and invalid UTF-8 byte sequences.
///
/// # Safety Contract
///
/// This function is marked `pub` but not `unsafe` because the unsafety is encapsulated within its
/// implementation. However, it relies on a strict contract with the FFI caller. The caller
/// **must** guarantee that the `ptr` argument satisfies one of two conditions:
///
/// 1.  It is a null pointer.
/// 2.  It is a valid pointer to the first byte of a null-terminated sequence of bytes that remains
///     valid for the lifetime of this function's execution.
///
/// The memory pointed to by `ptr` is treated as borrowed and is **not** deallocated by this function.
/// It remains the responsibility of the C/C++ caller to manage the memory of the C string it passes.
///
/// Violating this contract by providing a dangling pointer, a non-null-terminated sequence, or
/// a pointer to uninitialized memory will result in undefined behavior.
///
/// # Arguments
///
/// * `ptr` - A raw pointer to a constant, null-terminated C-style string.
///
/// # Returns
///
/// An owned Rust `String` containing the converted data.
///
/// In the case of an error (either a null input pointer or a C string that is not valid UTF-8),
/// this function returns a default error value: an empty JSON object string (`"{}"`). This design
/// choice avoids panicking at the FFI boundary and provides a consistent, albeit simple, error
/// signal to the consuming Rust logic.
///
/// # Example
///
/// ```
/// use libc::c_char;
/// use std::ffi::CString;
/// use std::ptr;
/// use aevum_ffi::util::from_c_str;
///
/// // Example with a valid C string.
/// let c_string = CString::new("A valid C string").unwrap();
/// let rust_string = from_c_str(c_string.as_ptr());
/// assert_eq!(rust_string, "A valid C string");
///
/// // Example with a null pointer, which should return the default error string.
/// let null_ptr: *const c_char = ptr::null();
/// let default_string = from_c_str(null_ptr);
/// assert_eq!(default_string, "{}");
/// ```
pub fn from_c_str(ptr: *const c_char) -> String {
    if ptr.is_null() {
        // A null pointer is a common way for C APIs to indicate "no value" or an error.
        // We handle this gracefully by returning a default, safe string value.
        return "{}".to_string();
    }

    // This block is `unsafe` because it involves dereferencing a raw pointer, which the Rust
    // compiler cannot prove is safe. We rely on the function's safety contract with the caller.
    unsafe {
        // `CStr::from_ptr` creates a safe, borrowed `&CStr` slice from the raw pointer.
        // This operation is itself unsafe as it trusts the pointer to be valid and null-terminated.
        CStr::from_ptr(ptr)
            // `.to_str()` attempts to convert the byte slice into a UTF-8 `&str`. This can fail
            // if the byte sequence is not valid UTF-8.
            .to_str()
            // If the UTF-8 conversion fails, `.unwrap_or("{}")` provides a default error value,
            // preventing a panic and ensuring the function always returns a valid string.
            .unwrap_or("{}")
            // Finally, `.to_string()` converts the `&str` slice into an owned `String`.
            .to_string()
    }
}

// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

use libc::c_char;
use std::ffi::CString;
use std::ptr;

/// Converts an owned Rust `String` into a C-compatible, heap-allocated, null-terminated string,
/// transferring ownership to the FFI caller.
///
/// This function is essential for passing string data from Rust to external C/C++ code. It takes
/// an owned `String`, attempts to convert it into a `CString` (which ensures it is null-terminated
/// and contains no interior null bytes), and then transfers ownership of the underlying buffer
/// to the caller by returning a raw pointer.
///
/// # Safety and Memory Management
///
/// The returned `*mut c_char` points to memory that has been allocated by Rust's global allocator.
/// The caller of this function across the FFI boundary receives ownership of this memory and is
/// therefore **contractually obligated** to deallocate it. This deallocation **must** be performed
/// by calling the corresponding `rust_free_string` function, which correctly returns the memory
/// to the Rust allocator. Failure to do so will result in a memory leak.
///
/// ## Error Handling
///
/// The C language string representation (`char*`) does not support interior null bytes (`\0`). If the
/// input `String` contains such a byte, `CString::new` will return an error. In this scenario, this
/// function will return a null pointer (`ptr::null_mut()`) to signal the failure to the caller. The
/// C/C++ caller **must** check for this null return value to handle the error appropriately.
///
/// # Arguments
///
/// * `s` - The owned Rust `String` to be converted and transferred.
///
/// # Returns
///
/// On success, returns a `*mut c_char` pointer to the start of the newly allocated, null-terminated
/// C-style string.
/// On failure (due to an interior null byte in the input `s`), returns `ptr::null_mut()`.
///
/// # Example
///
/// ```
/// use aevum_ffi::util::{to_c_string, rust_free_string};
/// use std::ffi::CStr;
///
/// // Case: Success
/// let rust_string = String::from("Hello from Rust");
/// let c_ptr = to_c_string(rust_string);
///
/// assert!(!c_ptr.is_null());
///
/// // The C/C++ side would now use `c_ptr`.
/// // We can simulate reading it back for verification.
/// unsafe {
///     let c_str = CStr::from_ptr(c_ptr);
///     assert_eq!(c_str.to_str().unwrap(), "Hello from Rust");
/// };
///
/// // The C/C++ side is now responsible for freeing the memory.
/// unsafe {
///     rust_free_string(c_ptr);
/// }
///
/// // Case: Error (Interior Null)
/// let invalid_rust_string = String::from("hello\0world");
/// let null_ptr = to_c_string(invalid_rust_string);
/// assert!(null_ptr.is_null());
/// ```
pub fn to_c_string(s: String) -> *mut c_char {
    match CString::new(s) {
        Ok(c_string) => c_string.into_raw(),
        Err(_) => {
            // `CString::new` fails if the input string contains an internal null byte.
            // In this case, we signal an error to the C caller by returning a null pointer,
            // which is a common convention in C APIs.
            ptr::null_mut()
        }
    }
}

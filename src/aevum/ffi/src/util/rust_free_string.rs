// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

use libc::c_char;
use std::ffi::CString;

/// Deallocates memory for a C-style string that was previously allocated and transferred by Rust.
///
/// This function is a critical component of the FFI memory management contract. It is exposed to
/// C/C++ so that foreign code can correctly return ownership of a Rust-allocated string back to
/// Rust for safe deallocation. It reverses the process of `CString::into_raw()`, which is used
/// internally by `to_c_string`.
///
/// # Safety
///
/// This function is profoundly `unsafe` as it involves taking ownership and deallocating memory
/// from a raw pointer. The caller **must** rigorously adhere to the following contract to prevent
/// severe memory corruption issues like double-frees or use-after-frees:
///
/// 1.  **Origin**: The `ptr` **must** be a pointer that was originally obtained from a call to
///     `CString::into_raw()` within this Rust library (specifically, via the `to_c_string` function).
///     Passing any other type of pointer (e.g., from `malloc`, `new`, a stack variable, or another
///     library) will result in undefined behavior, as Rust's allocator will be attempting to free
///     memory it does not manage.
/// 2.  **Uniqueness**: The `ptr` **must** be considered "live" and must not have been freed already.
///     Calling this function more than once on the same pointer is a double-free error, which is a
///     critical memory safety violation.
/// 3.  **Integrity**: The `ptr` must not have been altered since it was received from Rust.
///
/// A null pointer is handled safely as a no-op, which is a common convention in C APIs.
///
/// # Example (Illustrating the full memory lifecycle)
///
/// ```
/// use aevum_ffi::util::{to_c_string, rust_free_string};
///
/// // 1. Rust creates a String and prepares it for FFI.
/// let rust_str = String::from("This memory is managed by Rust");
/// let c_str_ptr = to_c_string(rust_str);
///
/// // 2. The pointer is "used" by the foreign code (simulated here).
/// //    assert!(!c_str_ptr.is_null());
///
/// // 3. The foreign code, having finished with the string, calls this function
/// //    to correctly deallocate the memory.
/// unsafe {
///     rust_free_string(c_str_ptr);
/// }
/// // `c_str_ptr` is now a dangling pointer and must not be used again.
/// ```
#[no_mangle]
pub unsafe extern "C" fn rust_free_string(ptr: *mut c_char) {
    if ptr.is_null() {
        // It is safe and conventional to do nothing for a null pointer.
        return;
    }
    // This is the core of the deallocation logic. `CString::from_raw` takes the raw pointer
    // and reconstitutes a `CString` object, reclaiming ownership of the memory.
    // Due to `#![deny(unsafe_op_in_unsafe_fn)]`, the operation must be explicitly wrapped
    // in an `unsafe` block even though the function itself is marked `unsafe`.
    unsafe {
        // The `let _ = ...` syntax is used to create a temporary variable that immediately goes
        // out of scope at the end of this statement. When it goes out of scope, Rust's ownership
        // rules trigger its destructor (`drop`), which in turn deallocates the memory that the
        // `CString` was managing.
        let _ = CString::from_raw(ptr);
    }
}

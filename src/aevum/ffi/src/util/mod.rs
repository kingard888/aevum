// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root directory.

//! # Core Foreign Function Interface (FFI) Utilities
//!
//! This module serves as a central nexus for essential utility functions that facilitate safe and
//! effective communication across the Rust-to-C/C++ boundary. The primary challenge in any FFI
//! layer is bridging the semantic gap between different language ecosystems, particularly with
//! respect to memory management, string representations, and error handling.
//!
//! This module and its sub-modules provide a robust and idiomatic Rust implementation for these
//! critical cross-language concerns, ensuring that the `aevum_ffi` crate is both powerful and
//! safe to consume from C/C++.
//!
//! ## Core Components
//!
//! - **`to_c_string`**: Provides a function to convert a Rust-native `String` into a heap-allocated,
//!   null-terminated C-style string, correctly transferring memory ownership to the C/C++ caller.
//!
//! - **`from_c_str`**: Provides a function to safely convert a C-style string pointer (`*const c_char`)
//!   into an owned Rust `String`, with robust handling of null pointers and potential UTF-8 errors.
//!
//! - **`rust_free_string`**: Exposes a C-callable function that allows the foreign code to return
//!   ownership of a Rust-allocated string back to Rust for proper deallocation, thus preventing
//!   memory leaks.
//!
//! By gathering and re-exporting these functionalities, this module creates a clean, consolidated,
//! and easily discoverable API surface for all FFI-related utilities.

/// Provides the function `to_c_string` for converting an owned Rust `String` into a C-compatible,
/// heap-allocated, null-terminated string, transferring ownership to the caller.
pub mod to_c_string;

/// Provides the function `from_c_str` for safely converting a raw C-style string pointer
/// into an owned Rust `String`, with built-in error handling for nulls and invalid UTF-8.
pub mod from_c_str;

/// Provides the C-callable function `rust_free_string` for deallocating a C-style string that was
/// previously allocated by this Rust FFI layer, completing the memory management contract.
pub mod rust_free_string;

// Re-export all public functions from the sub-modules. This ergonomic choice allows consumers
// of the `util` module to access these critical functions directly (e.g., `util::to_c_string`)
// without needing to reference the specific sub-module in which they are defined.
pub use from_c_str::from_c_str;
pub use rust_free_string::rust_free_string;
pub use to_c_string::to_c_string;

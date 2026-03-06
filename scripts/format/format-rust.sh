#!/bin/bash

##
# format-rust.sh
#
# This script applies Rust code formatting to the AevumDB FFI crate,
# ensuring consistency with the project's rustfmt.toml configuration.
#
# It specifically navigates to the FFI crate's directory and invokes
# `cargo fmt`, which is the standard command for formatting Rust projects.
##

set -e

# Define the root directory of the script to robustly locate the FFI crate.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." &> /dev/null && pwd)"
FFI_CRATE_DIR="$PROJECT_ROOT/src/aevum/ffi"

echo "Formatting Rust code in the FFI crate at '$FFI_CRATE_DIR'..."

# Navigate to the FFI crate directory and run `cargo fmt`.
# The --all flag ensures that all packages and targets within the crate are formatted.
(cd "$FFI_CRATE_DIR" && cargo fmt --all)

echo "Rust formatting complete."

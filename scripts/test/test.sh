#!/bin/bash

##
# test.sh
#
# This script runs cargo tests for the FFI module in AevumDB.
# It executes all tests in the src/aevum/ffi directory, ensuring proper
# isolation and comprehensive test coverage for the Foreign Function Interface.
#
# Usage:
#   ./test.sh                     # Run all tests
#   ./test.sh -- --test-name      # Run specific test
#   ./test.sh -- --nocapture      # Run tests with output
##

set -e

# Define the root directory of the project based on script location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." &> /dev/null && pwd)"
FFI_DIR="${PROJECT_ROOT}/src/aevum/ffi"

# Check if FFI directory exists
if [[ ! -d "$FFI_DIR" ]]; then
    echo "Error: FFI directory not found at $FFI_DIR"
    exit 1
fi

# Check if Cargo.toml exists in FFI directory
if [[ ! -f "$FFI_DIR/Cargo.toml" ]]; then
    echo "Error: Cargo.toml not found in $FFI_DIR"
    exit 1
fi

echo "Running cargo tests for AevumDB FFI module..."
echo ""

# Execute cargo test in the FFI directory
cd "$FFI_DIR"
cargo test "$@"

echo ""
echo "FFI tests completed."

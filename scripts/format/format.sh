#!/bin/bash

##
# format.sh
#
# This script is the main entry point for formatting all code in the AevumDB
# project. It sequentially invokes the specialized formatting scripts for C++
# and Rust, providing a single command to ensure the entire codebase adheres
# to its defined coding style conventions.
##

set -e

# Define the directory where this script is located to robustly call other scripts.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"

echo "Starting AevumDB Code Formatter"

# Execute the C++ formatting script.
echo ""
echo "Running C++ formatter..."
"$SCRIPT_DIR/format-cpp.sh"

# Execute the Rust formatting script.
echo ""
echo "Running Rust formatter..."
"$SCRIPT_DIR/format-rust.sh"

echo ""
echo "All formatting complete."

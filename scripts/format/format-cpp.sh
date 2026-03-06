#!/bin/bash

##
# format-cpp.sh
#
# This script applies C++ code formatting to the entire AevumDB project,
# ensuring a consistent and conventional coding style across all C++ source
# and header files. It uses clang-format with the project's root .clang-format
# configuration file.
#
# The script first identifies all relevant .cpp and .hpp files within the 'src/aevum'
# directory and then executes clang-format in-place on each file.
##

set -e

# Define the root directory of the script to robustly locate project files.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." &> /dev/null && pwd)"

echo "Searching for C++ files to format in '$PROJECT_ROOT/src/aevum'..."

# Find all .cpp and .hpp files within the src/aevum directory, excluding
# any third-party directories that might be present.
# The -i flag for clang-format applies the formatting changes in-place.
find "$PROJECT_ROOT/src/aevum" -type f \( -name "*.cpp" -o -name "*.hpp" \) -print0 | while IFS= read -r -d $'\0' file; do
    echo "Formatting $file..."
    clang-format -i "$file"
done

echo "C++ formatting complete."

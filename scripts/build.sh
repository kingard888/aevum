#!/bin/bash

##
# build.sh
#
# Convenience wrapper for running the build script from the scripts root directory.
# This redirects to scripts/build/build.sh for actual build operations.
##

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
bash "$SCRIPT_DIR/build/build.sh" "$@"

#!/bin/bash

##
# test.sh
#
# Convenience wrapper for running the test script from the scripts root directory.
# This redirects to scripts/test/test.sh for actual test operations.
##

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
bash "$SCRIPT_DIR/test/test.sh" "$@"

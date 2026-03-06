#!/bin/bash

##
# format.sh
#
# Convenience wrapper for running the format script from the scripts root directory.
# This redirects to scripts/format/format.sh for actual formatting operations.
##

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
bash "$SCRIPT_DIR/format/format.sh" "$@"

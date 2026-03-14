#!/bin/bash

##
# install.sh
#
# Convenience wrapper for running the installation script from the scripts root directory.
# This redirects to scripts/install/install.sh for actual installation operations.
##

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
bash "$SCRIPT_DIR/install/install.sh" "$@"

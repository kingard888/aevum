#!/bin/bash

##
# install.sh
#
# This script performs a full lifecycle check and installs AevumDB to the system.
# It handles formatting, linting, testing, building, and system-wide deployment.
#
# Usage:
#   sudo ./scripts/install.sh
##

set -e

# Define colors for feedback
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check for root privileges
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}Error: Please run as root (use sudo).${NC}"
    exit 1
fi

# Determine project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." &> /dev/null && pwd)"

# Function to run commands as the original user if sudo is used
run_as_user() {
    if [ -n "$SUDO_USER" ]; then
        # Dynamically resolve the original user's home directory
        local original_home
        original_home=$(getent passwd "$SUDO_USER" | cut -d: -f6)
        # Run command as original user with their correct HOME and PATH
        sudo -u "$SUDO_USER" env PATH="$PATH" HOME="$original_home" "$@"
    else
        "$@"
    fi
}

echo -e "${BLUE}Initiating AevumDB Full Lifecycle Check & Installation...${NC}"

# 1. Format Code
echo "Step 1: Formatting code..."
run_as_user bash "$PROJECT_ROOT/scripts/format.sh"

# 2. Build Project (Required for linting compile_commands.json)
echo "Step 2: Building project..."
run_as_user bash "$PROJECT_ROOT/scripts/build.sh"

# 3. Lint Code
echo "Step 3: Linting code..."
run_as_user bash "$PROJECT_ROOT/scripts/lint.sh"

# 4. Run Tests
echo "Step 4: Running tests..."
run_as_user bash "$PROJECT_ROOT/scripts/test.sh"

# Define installation paths
INSTALL_PREFIX="/opt/aevumdb"
BIN_DIR="$INSTALL_PREFIX/bin"
DATA_DIR="$INSTALL_PREFIX/data"
LOG_DIR="$INSTALL_PREFIX/log"
CONF_DIR="/etc/aevum"
SYMLINK_DIR="/usr/local/bin"
BUILD_DIR="${PROJECT_ROOT}/build"

echo -e "${BLUE}Step 5: Deploying to system...${NC}"

# Create directory structure
echo "Creating directory structure..."
mkdir -p "$BIN_DIR"
mkdir -p "$DATA_DIR"
mkdir -p "$LOG_DIR"
mkdir -p "$CONF_DIR"

# Copy binaries
echo "Installing binaries..."
cp -f "$BUILD_DIR/bin/aevumdb" "$BIN_DIR/"
cp -f "$BUILD_DIR/bin/aevumsh" "$BIN_DIR/"
chmod +x "$BIN_DIR/aevumdb" "$BIN_DIR/aevumsh"

# Create a default configuration if it doesn't exist
if [ ! -f "$CONF_DIR/aevumdb.conf" ]; then
    echo "Creating default configuration in $CONF_DIR..."
    cat <<EOF > "$CONF_DIR/aevumdb.conf"
# AevumDB Default Configuration
storage:
  dbPath: $DATA_DIR
systemLog:
  destination: file
  path: $LOG_DIR/aevumdb.log
net:
  port: 55001
  bindIp: 127.0.0.1
EOF
fi

# Create global symlinks
echo "Creating global symlinks..."
ln -sf "$BIN_DIR/aevumdb" "$SYMLINK_DIR/aevumdb"
ln -sf "$BIN_DIR/aevumsh" "$SYMLINK_DIR/aevumsh"

# Register systemd service
SERVICE_FILE="$PROJECT_ROOT/scripts/install/aevumdb.service"
if [ -f "$SERVICE_FILE" ]; then
    echo "Registering AevumDB as a system service..."
    cp -f "$SERVICE_FILE" /etc/systemd/system/
    systemctl daemon-reload
fi

# Set permissions
chmod 777 "$DATA_DIR"
chmod 777 "$LOG_DIR"

echo ""
echo -e "${GREEN}AevumDB has been installed successfully!${NC}"
echo ""
echo -e "  Installation Path : $INSTALL_PREFIX"
echo -e "  Config Path       : $CONF_DIR/aevumdb.conf"
echo -e "  Data Directory    : $DATA_DIR"
echo -e "  Log Directory     : $LOG_DIR"
echo ""
echo -e "To manage the AevumDB service:"
echo -e "  sudo systemctl start aevumdb"
echo -e "  sudo systemctl enable aevumdb"
echo -e "  systemctl status aevumdb"
echo ""
echo -e "To connect to the database shell:"
echo -e "  aevumsh"
echo ""

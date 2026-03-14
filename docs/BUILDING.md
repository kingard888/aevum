# Building AevumDB

Complete guide to building AevumDB from source with various configuration options.

## Quick Build & Install

AevumDB includes an advanced build and installation suite that automatically optimizes your environment.

```bash
# Make all scripts executable
chmod +x ./scripts/*.sh ./scripts/*/*.sh

# Full Lifecycle Installation (Recommended)
# This will: Format -> Build -> Lint -> Test -> Install
sudo ./scripts/install.sh
```

## Prerequisites

The build script (`./scripts/build.sh`) automatically detects and installs missing dependencies for you.

### Supported Distributions
- **Arch Linux** (pacman)
- **Ubuntu/Debian** (apt)
- **Fedora/RHEL** (dnf)

### Required Tools
- **C++ Compiler**: GCC or Clang (C++17 support)
- **Rust**: For the Query Execution Engine (FFI)
- **CMake**: version 3.21+
- **Ninja**: Highly recommended for build speed (default if available)
- **ccache**: Recommended for near-instant rebuilds

## Optimization Features

### Persistent Vendor Strategy
AevumDB now builds third-party libraries (WiredTiger, libbson) into a persistent `third_party/dist` directory. This means:
- You can run `rm -rf build/` without losing pre-built libraries.
- Subsequent environment setups are near-instant.

### Advanced C++ Compilation
- **Unity Build**: Batches source files to reduce compiler overhead.
- **Precompiled Headers (PCH)**: Pre-compiles heavy headers (STL, BSON, simdjson) only once.
- **Core Library**: Consolidates logic into `aevum_core` to avoid double compilation.

## Build Process (Manual)

If you prefer to build manually without the installation script:

```bash
# Standard optimized build using the orchestrator
./scripts/build.sh

# Or clean and rebuild
./scripts/build.sh rebuild
```

### Ninja Manual Build
```bash
mkdir -p build && cd build
cmake -G Ninja ..
ninja -j$(nproc)
```

## Installation

### System-wide Deployment (/opt/aevumdb)
This follows the professional database model (like MongoDB).

```bash
sudo ./scripts/install.sh
```

**Results:**
- **Binaries**: `/opt/aevumdb/bin/aevumdb`, `/opt/aevumdb/bin/aevumsh`
- **Configuration**: `/etc/aevum/aevumdb.conf`
- **Data/Logs**: `/opt/aevumdb/data`, `/opt/aevumdb/log`
- **System Service**: `aevumdb.service` registered in systemd.
- **Global Access**: `aevumdb` and `aevumsh` available in your PATH.

## Build Modes

### Debug Build
```bash
./scripts/build.sh debug  # (If supported by script)
# Or manually:
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

### Release Build (Default)
```bash
./scripts/build.sh
# Or manually:
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Performance Tuning

### Compiler Optimization
Release mode uses `-O3` by default. To target your specific CPU architecture:
```bash
cmake -DCMAKE_CXX_FLAGS="-march=native" ..
```

### Parallel Jobs
By default, the build script uses all available CPU cores. To limit them:
```bash
./scripts/build.sh -j 4
```

## Common Build Issues

### stoi Exception on Startup
If the daemon fails with `stoi`, ensure your `/etc/aevum/aevumdb.conf` contains a valid numerical port and that you are using the binary which supports the `--config` flag.

### Missing ccache or Ninja
The build script will attempt to install them. If it fails, install them manually:
- `sudo apt install ninja-build ccache`
- `sudo dnf install ninja-build ccache`
- `sudo pacman -S ninja ccache`

## See Also
- [Scripts Documentation](SCRIPTS.md)
- [Deployment Guide](DEPLOYMENT.md)
- [Architecture Overview](ARCHITECTURE.md)

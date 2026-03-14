# Scripts Documentation

AevumDB includes a comprehensive script suite for the full development lifecycle: formatting, building, analysis, testing, and deployment.

## Quick Reference

```bash
# Full Installation (Recommended)
# Automatically runs: Format -> Build -> Lint -> Test -> Install
sudo ./scripts/install.sh

# Build the project (default: Release mode)
./scripts/build.sh

# Clean and rebuild
./scripts/build.sh rebuild

# Format C++ and Rust code
./scripts/format.sh

# Run code analysis (clang-tidy)
./scripts/lint.sh

# Run FFI tests (cargo test)
./scripts/test.sh

# Show help
./scripts/build.sh help
```

## Installation Scripts

### scripts/install.sh

Main installation orchestrator. This script is designed for centralized system deployment and ensures the codebase is in perfect condition before installation.

**Location**: `/scripts/install.sh`

**Usage**:
```bash
sudo ./scripts/install.sh
```

**What it does**:
1. Redirects to the implementation in `scripts/install/install.sh`.
2. Initiates the **Full Lifecycle Check**:
   - **Step 1**: Formats code using `format.sh`.
   - **Step 2**: Builds the project using `build.sh` (required for linting).
   - **Step 3**: Lints code using `lint.sh` to ensure quality.
   - **Step 4**: Runs Rust FFI tests using `test.sh`.
   - **Step 5**: Deploys AevumDB to `/opt/aevumdb/` and registers the systemd service.

### scripts/install/install.sh

Core installation implementation. Handles file system organization and service registration.

**Location**: `/scripts/install/install.sh`

**Standard Install Paths**:
- **Binaries**: `/opt/aevumdb/bin/`
- **Data**: `/opt/aevumdb/data/`
- **Logs**: `/opt/aevumdb/log/`
- **Configuration**: `/etc/aevum/aevumdb.conf`
- **Global Commands**: `/usr/local/bin/` (symlinks)
- **System Service**: `aevumdb.service`

## Build Scripts

### scripts/build.sh

Main build orchestrator. Optimized for performance using Ninja, ccache, and persistent vendor caching.

**Location**: `/scripts/build.sh`

**Commands**:
- `(default)`: Configure and build the project.
- `clean`: Remove build artifacts.
- `rebuild`: Clean and then build fresh.
- `verbose`: Show detailed compiler output.

**What it does**:
1. Detects and installs missing build tools (Ninja, ccache, CMake).
2. Automatically prioritizes the **Ninja** build system.
3. Utilizes **ccache** to accelerate recompilation.
4. Manages the persistent vendor directory (`third_party/dist`).

## Formatting Scripts

### scripts/format.sh

Code formatting orchestrator for both C++ and Rust.

**Usage**:
```bash
./scripts/format.sh [cpp|rust]
```

- **cpp**: Uses `clang-format` on all `.cpp` and `.hpp` files.
- **rust**: Uses `rustfmt` on all `.rs` files in the FFI module.

## Linting Scripts

### scripts/lint.sh

Static analysis orchestrator using `clang-tidy`.

**Usage**:
```bash
./scripts/lint.sh [TARGET] [--fix]
```

**What it does**:
- Analyzes C++ code for bugs, style issues, and modern standards.
- Requires `compile_commands.json` generated during the build process.

## Testing Scripts

### scripts/test.sh

FFI test orchestrator for the Query Execution Engine.

**Usage**:
```bash
./scripts/test.sh [CARGO_ARGS]
```

**What it does**:
- Executes `cargo test` in the `src/aevum/ffi` directory.
- Verifies the integrity of the C++/Rust interface.

## Directory Structure

```
scripts/
├── build.sh                 # Build orchestrator
├── install.sh               # Install orchestrator
├── format.sh                # Format orchestrator
├── lint.sh                  # Lint orchestrator
├── test.sh                  # Test orchestrator
│
├── build/
│   └── build.sh            # Build core logic
├── install/
│   ├── install.sh          # Install core logic
│   └── aevumdb.service     # Systemd definition
├── format/
│   ├── format-cpp.sh       # C++ format implementation
│   └── format-rust.sh      # Rust format implementation
├── lint/
│   └── lint.sh             # Lint implementation
└── test/
    └── test.sh             # Test implementation
```

## Troubleshooting

### Script Permission Denied
Ensure all scripts are executable:
```bash
chmod +x scripts/*.sh scripts/*/*.sh
```

### Build fails with ccache error
Ensure ccache is installed or clear its cache:
```bash
ccache -C
```

### Installation fails (No sudo)
The installation script requires root privileges to write to `/opt` and `/etc`:
```bash
sudo ./scripts/install.sh
```

## See Also
- [Building AevumDB](BUILDING.md)
- [Deployment Guide](DEPLOYMENT.md)
- [Project Structure](PROJECT_STRUCTURE.md)

# Project Structure

AevumDB is organized into logical directories. This guide explains what each directory contains and how they relate to each other.

## Root Level

```
aevumdb/
├── README.md               # Main project overview
├── LICENSE                 # Community license
├── CMakeLists.txt         # CMake build configuration
├── docs/                  # Documentation
├── scripts/               # Build, install, and utility scripts
├── src/                   # Source code
├── third_party/           # External dependencies
│   └── dist/              # Persistent pre-built libraries (v1.2.0+)
├── build/                 # Build output (generated)
└── .git/                  # Git repository metadata
```

## /docs - Documentation

Complete user and developer documentation.

```
docs/
├── README.md              # Documentation index
├── GETTING_STARTED.md     # 5-minute quick start for new users
├── SHELL_REFERENCE.md     # Complete interactive shell commands
├── ARCHITECTURE.md        # System design, components, data flow
├── DEVELOPMENT.md         # Development environment setup and workflow
├── BUILDING.md            # Build from source with various options
├── DEPLOYMENT.md          # Production deployment and operations
├── API.md                 # C++ client library API reference
└── TROUBLESHOOTING.md     # Solutions to common problems
```

**See** [docs/README.md](README.md) for documentation index.

## /scripts - Build, Install, and Utilities

Automation and development scripts.

```
scripts/
├── build.sh              # Main build orchestrator
├── install.sh            # Main installation orchestrator (v1.2.0+)
├── format.sh             # Code formatting orchestrator
├── lint.sh               # Code analysis (clang-tidy) orchestrator
├── test.sh               # FFI testing orchestrator
├── build/
│   └── build.sh          # Core build logic (Ninja/ccache prioritized)
├── install/
│   ├── install.sh        # Core installation logic (/opt/aevumdb)
│   └── aevumdb.service   # systemd service definition
├── format/
│   ├── format-cpp.sh     # C++ formatting
│   └── format-rust.sh    # Rust formatting
├── lint/
│   └── lint.sh           # C++ analysis implementation
└── test/
    └── test.sh           # Rust FFI test implementation
```

**See** [docs/SCRIPTS.md](SCRIPTS.md) for detailed script documentation.

## /src - Source Code

Application source code organized by component.

```
src/aevum/
├── main.cpp              # Daemon server entry point (config parsing enabled)
│
├── db/                   # Database engine core
│   ├── core/             # Core engine logic
│   ├── storage/          # Physical data persistence
│   ├── auth/             # Authentication and authorization
│   ├── schema/           # Schema validation
│   └── index/            # Query indexing
│
├── shell/                # Interactive shell client
│   ├── main.cpp          # Shell entry point
│   ├── repl/             # Read-Eval-Print Loop
│   └── parser/           # Command parsing
│
├── client/               # Client library
│
├── bson/                 # BSON document serialization
│
└── util/                 # Utility functions
    ├── log/              # Logging utilities
    ├── memory/           # Memory management
    ├── status.hpp        # Status/error codes
    └── ...               # Other utilities
```

**See** [docs/DEVELOPMENT.md](DEVELOPMENT.md) for development guide.

## /third_party - External Dependencies

External libraries and their persistent pre-built artifacts.

```
third_party/
├── dist/                 # Pre-built artifacts (survives 'rm -rf build')
│   ├── bin/
│   ├── include/
│   └── lib/              # libwiredtiger.a, libbson-static-1.0.a
│
├── wiredtiger/           # Storage engine source
├── mongo-c-driver/       # BSON library source
└── simdjson/             # JSON parser source
```

**See** [docs/THIRD_PARTY.md](THIRD_PARTY.md) for external dependencies.

## /build - Build Output (Generated)

Created when building the project. Safe to delete.

```
build/
├── bin/                  # Compiled binaries (aevumdb, aevumsh)
├── lib/                 # Library files
└── ...                  # CMake artifacts
```

## System Installation Paths (v1.2.0+)

When using `sudo ./scripts/install.sh`:

```
/opt/aevumdb/
├── bin/                  # System binaries
├── data/                 # Default database storage
└── log/                  # System logs

/etc/aevum/
└── aevumdb.conf          # System-wide configuration

/usr/local/bin/
├── aevumdb               # Symlink to /opt/aevumdb/bin/aevumdb
└── aevumsh               # Symlink to /opt/aevumdb/bin/aevumsh
```

## Configuration Files

### Build
- `CMakeLists.txt` - Main build configuration

### System
- `/etc/aevum/aevumdb.conf` - Daemon configuration (YAML)

### Code Style
- `.clang-format` - C++ formatting rules
- `rustfmt.toml` - Rust formatting config

## Key Relationships

```
┌─────────────────────────────────────────────────────┐
│  README.md (Root - Overview & Quick Links)          │
└──────────────────┬──────────────────────────────────┘
                   │
    ┌──────────────┼──────────────┬──────────────┐
    │              │              │              │
    ▼              ▼              ▼              ▼
 /docs/        /scripts/        /src/      /third_party/
Building    Build/Install    Source code   Persistent
 & running  Formatting        Core apps    Vendor
 Database   Analysis          Auth        Strategy
            Testing           Storage     (libbson, WT)
```

## Build Process Flow

```
CMake (Ninja/ccache)
    ↓
    ├─→ Checks third_party/dist for pre-built artifacts
    │   └─ If missing, builds WiredTiger and libbson once
    │
    ├─→ Generates aevum_core static library
    │   ├─ Unity Build enabled (batching)
    │   └─ Precompiled Headers (PCH) enabled
    │
    └─→ Links into binaries
        ├─ aevumdb (Daemon)
        └─ aevumsh (Shell)
```

## Common Paths Reference

| What | Location |
|------|----------|
| Server binary | `build/bin/aevumdb` |
| Shell binary | `build/bin/aevumsh` |
| System Install | `/opt/aevumdb` |
| System Config | `/etc/aevum/aevumdb.conf` |
| Source code | `src/aevum/` |
| Build script | `scripts/build.sh` |
| Install script | `scripts/install.sh` |

## See Also

- [Building](BUILDING.md) - How to compile
- [Deployment](DEPLOYMENT.md) - System installation
- [Scripts](SCRIPTS.md) - Build/Install/Utility scripts
- [Architecture](ARCHITECTURE.md) - How it works

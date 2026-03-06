# Project Structure

AevumDB is organized into logical directories. This guide explains what each directory contains and how they relate to each other.

## Root Level

```
aevumdb/
├── README.md               # Main project overview
├── LICENSE                 # Community license
├── CMakeLists.txt         # CMake build configuration
├── docs/                  # Documentation
├── scripts/               # Build and utility scripts
├── src/                   # Source code
├── third_party/           # External dependencies
├── build/                 # Build output (generated)
├── data/                  # Database storage (generated)
├── dev/                   # Development database (generated)
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

## /scripts - Build and Utilities

Build automation and development scripts.

```
scripts/
├── build.sh              # Main build orchestrator
│                         # Usage: ./scripts/build.sh [clean|rebuild|verbose]
├── format.sh             # Code formatting orchestrator
│                         # Usage: ./scripts/format.sh [cpp|rust]
├── build/
│   └── build.sh          # Core build logic
│                         # Handles CMake configuration and make
└── format/
    ├── format.sh         # Formatting orchestrator
    ├── format-cpp.sh     # C++ formatting with clang-format
    └── format-rust.sh    # Rust formatting with rustfmt
```

**See** [docs/BUILDING.md](BUILDING.md) for detailed build instructions.

## /src - Source Code

Application source code organized by component.

```
src/aevum/
├── main.cpp              # Daemon server entry point
│                         # - Initializes database engine
│                         # - Starts network server
│                         # - Handles POSIX signals
│
├── db/                   # Database engine core
│   ├── core/
│   │   ├── core.hpp      # Core class (main orchestrator)
│   │   └── core.cpp      # Core implementation
│   │
│   ├── storage/          # Physical data persistence
│   │   └── wiredtiger_store.hpp  # WiredTiger integration
│   │
│   ├── auth/             # Authentication and authorization
│   │   └── auth_manager.hpp      # User and role management
│   │
│   ├── schema/           # Schema validation
│   │   └── schema_manager.hpp    # Schema definitions
│   │
│   └── index/            # Query indexing
│       └── index_manager.hpp     # Index management
│
├── shell/                # Interactive shell client
│   ├── main.cpp          # Shell entry point
│   ├── repl/
│   │   ├── repl.hpp      # Read-Eval-Print Loop
│   │   └── repl.cpp      # REPL implementation
│   └── parser/
│       ├── command_parser.hpp    # Command parsing
│       └── command_parser.cpp    # Parser implementation
│
├── client/               # Client library
│   ├── aevum_client.hpp  # High-level C++ API
│   ├── aevum_client.cpp  # Client implementation
│   └── net/
│       ├── server.hpp    # Network server (daemon)
│       ├── connection.hpp # TCP connection (client)
│       └── ...           # Network utilities
│
├── bson/                 # BSON document serialization
│   ├── doc/
│   │   ├── document.hpp  # Document representation
│   │   └── builder.hpp   # Document builder
│   └── json/
│       └── json.hpp      # JSON conversion utilities
│
└── util/                 # Utility functions
    ├── log/              # Logging utilities
    ├── memory/           # Memory management (arena allocator)
    ├── concurrency/      # Thread synchronization
    ├── string/           # String utilities
    ├── status.hpp        # Status/error codes
    └── ...               # Other utilities
```

**See** [docs/DEVELOPMENT.md](DEVELOPMENT.md) for development guide.

## /third_party - External Dependencies

External libraries built as part of the project.

```
third_party/
├── wiredtiger/
│   ├── CMakeLists.txt
│   ├── src/              # Storage engine source
│   ├── include/          # Public headers
│   └── ...
│
├── mongo-c-driver/
│   ├── CMakeLists.txt
│   ├── src/              # BSON library source
│   ├── src/bson/         # BSON implementation
│   └── ...
│
└── simdjson/
    ├── CMakeLists.txt
    ├── src/              # JSON parser source
    ├── include/          # Public headers
    └── ...
```

**See** [docs/THIRD_PARTY.md](THIRD_PARTY.md) for external dependencies.

## /build - Build Output (Generated)

Created when building the project.

```
build/
├── bin/                  # Compiled binaries
│   ├── aevumdb          # Database server daemon
│   └── aevumsh          # Interactive shell
├── lib/                 # Library files
├── CMakeFiles/          # CMake build files
├── Makefile             # Generated Makefile
└── cmake_install.cmake  # CMake install script
```

Regenerated on each build. Safe to delete: `rm -rf build`

## /data - Database Storage (Generated)

Default location for persistent database files.

```
data/
├── database.wt          # Main WiredTiger database file
├── WiredTiger           # WiredTiger metadata
├── WiredTiger.basecfg   # WiredTiger base configuration
├── WiredTiger.turtle    # WiredTiger turtle file
├── WiredTiger.lock      # Lock file
└── ... other WiredTiger files
```

Location can be customized with `--data-dir` option.

## /dev - Development Database (Generated)

Temporary development database for testing and exploration.

```
dev/
├── _auth.wt             # Authentication data
├── users.wt             # User collection
├── _schemas.wt          # Schema definitions
└── ... other data files
```

Created during development testing. Safe to delete.

## File Naming Conventions

### C++ Files
- **Headers**: `.hpp` extension
- **Implementation**: `.cpp` extension
- Example: `core.hpp` and `core.cpp`

### Source Organization
- **Classes**: One per file pair (hpp/cpp)
- **Namespaces**: Organized hierarchically
  - `aevum::db::core::Core`
  - `aevum::shell::repl`
  - `aevum::util::log`

### Documentation Files
- **Markdown**: `.md` extension
- **Index**: `README.md`
- **Guides**: `GUIDE_NAME.md` (e.g., `BUILDING.md`)

## Configuration Files

### Build
- `CMakeLists.txt` - Main build configuration
- `cmake/` subdirectories - CMake helpers

### Code Style
- `.clang-format` - C++ formatting rules
- `rustfmt.toml` - Rust formatting config

### Version Control
- `.git/` - Git repository
- `.gitignore` - Ignored files

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
Building    Build system     Source code   Dependencies
 & running  Formatting        Core apps    WiredTiger
 Database   Cleanup           Auth        BSON
            Development       Storage     JSON Parser
```

## Build Process Flow

```
CMakeLists.txt
    ↓
    ├─→ Configures third_party/ libraries
    │   ├─ mongo-c-driver (BSON)
    │   ├─ simdjson (JSON parsing)
    │   └─ wiredtiger (Storage)
    │
    ├─→ Compiles src/ code
    │   ├─ db/ (Database engine)
    │   ├─ client/ (Client library)
    │   └─ shell/ (Interactive shell)
    │
    └─→ Links into binaries
        ├─ build/bin/aevumdb (Server)
        └─ build/bin/aevumsh (Shell)
```

## Common Paths Reference

| What | Location |
|------|----------|
| Server binary | `build/bin/aevumdb` |
| Shell binary | `build/bin/aevumsh` |
| Documentation | `docs/` |
| Source code | `src/aevum/` |
| Database files | `data/` (default) |
| Build output | `build/` |
| Build script | `scripts/build.sh` |
| Format script | `scripts/format.sh` |

## See Also

- [Building](BUILDING.md) - How to compile
- [Development](DEVELOPMENT.md) - How to develop
- [Architecture](ARCHITECTURE.md) - How it works
- [THIRD_PARTY.md](THIRD_PARTY.md) - External libraries

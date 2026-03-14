# Third-Party Libraries

AevumDB uses three major third-party libraries for core functionality. All are included in the `/third_party/` directory and built as part of the project.

## WiredTiger

**Purpose**: Embedded key-value storage engine  
**Location**: `third_party/wiredtiger/`  
**License**: SSPL / GPL  
**Website**: http://source.wiredtiger.com/

### What it does
- Provides persistent data storage
- Handles transactions and ACID properties
- Manages database files and caching
- Supports compression and indexing

### Used by
- `src/aevum/db/storage/wiredtiger_store.hpp` - Storage layer
- All persistent data in `data/` directory

### How it's built
AevumDB uses a **Persistent Vendor Strategy** for heavy third-party libraries.

1. **WiredTiger**: Built using `ExternalProject_Add` and installed into `third_party/dist`.
2. **libbson**: Built as a subdirectory during the first run and installed into `third_party/dist`.

**Benefits**:
- **Persistence**: Results in `third_party/dist` are NOT deleted when you run `rm -rf build/`.
- **Speed**: Subsequent builds skip recompiling these heavy libraries, saving several minutes of build time.

### Configuration
In `CMakeLists.txt`, these libraries are integrated as `IMPORTED` targets once they are found in the `dist` directory.

### Configuration
In `CMakeLists.txt`~line 55-84:
```cmake
ExternalProject_Add(wiredtiger_project
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/third_party/wiredtiger"
    CMAKE_ARGS
        -DENABLE_STATIC:BOOL=ON
        -DENABLE_PYTHON:BOOL=OFF
        # ... other settings
)
```

## mongo-c-driver (BSON)

**Purpose**: BSON serialization library  
**Location**: `third_party/mongo-c-driver/`  
**License**: Apache 2.0  
**Website**: https://github.com/mongodb/mongo-c-driver

### What it does
- Serializes documents to BSON format (binary)
- Deserializes BSON back to memory structures
- Provides Document, Builder, Iterator interfaces
- Fast and efficient binary encoding

### Used by
- `src/aevum/bson/doc/document.hpp` - Document model
- `src/aevum/bson/doc/builder.hpp` - Document construction
- Network communication (BSON on wire)
- Persistent storage (BSON format)

### How it's built
```bash
# CMakeLists.txt enables only BSON component
# Sets: ENABLE_BSON=ON, ENABLE_MONGOC=OFF
# Linked statically as libbson.a
```

### Configuration
In `CMakeLists.txt`~line 39-50:
```cmake
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(ENABLE_MONGOC OFF CACHE BOOL "Build mongoc" FORCE)
set(ENABLE_BSON ON CACHE BOOL "Build bson" FORCE)
set(ENABLE_TESTS OFF CACHE BOOL "Build tests" FORCE)
```

## simdjson

**Purpose**: High-speed JSON parsing library  
**Location**: `third_party/simdjson/`  
**License**: Apache 2.0, MIT (dual)  
**Website**: https://simdjson.org/

### What it does
- Parses JSON text extremely fast (using SIMD)
- DOM-based tree representation
- Supports incremental parsing
- Used for shell communication

### Used by
- `src/aevum/shell/parser/command_parser.cpp` - Command parsing
- `src/aevum/bson/json/json.hpp` - JSON utilities
- Network communication (JSON over TCP)
- Shell output formatting

### How it's built
```bash
# CMakeLists.txt: add_subdirectory(third_party/simdjson)
# Integrated directly (header-heavy library)
# Linked statically as libsimdjson.a
```

## Build Integration

### CMakeLists.txt Snippet

```cmake
# Line ~32-50: mongo-c-driver
add_subdirectory(third_party/mongo-c-driver EXCLUDE_FROM_ALL)

# Line ~33-36: simdjson
add_subdirectory(third_party/simdjson EXCLUDE_FROM_ALL)

# Line ~52-84: WiredTiger (ExternalProject)
ExternalProject_Add(wiredtiger_project ...)
```

### Linking

All binaries link against these libraries:

```cmake
# For aevumdb daemon
target_link_libraries(aevumdb
    ... 
    wiredtiger
    bson_static
    simdjson
)

# For aevumsh shell
target_link_libraries(aevumsh
    ...
    bson_static
    simdjson
)
```

## Version Management

### Current Versions
- **WiredTiger**: 11.x (see `CMakeFiles/` after build)
- **mongo-c-driver**: Latest from `third_party/`
- **simdjson**: Latest from `third_party/`

### Updating Libraries

To update a library:

1. **Pull latest version**
   ```bash
   cd third_party/libraryname
   git pull  # or update via submodule
   ```

2. **Rebuild**
   ```bash
   rm -rf build
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```

3. **Test thoroughly**
   ```bash
   ./build/bin/aevumdb
   ./build/bin/aevumsh
   ```

## How Data Flows Through Libraries

### Write Path
```
User Input (JSON)
    ↓ (simdjson parses)
Parsed JSON
    ↓ (data extracted)
Document Structure
    ↓ (BSON encodes)
Binary BSON
    ↓ (WiredTiger stores)
Disk Storage
```

### Read Path
```
Disk Storage
    ↓ (WiredTiger retrieves)
Binary BSON
    ↓ (BSON decodes)
Document Structure
    ↓ (simdjson formats)
Output JSON
    ↓
User Output
```

## Performance Considerations

### WiredTiger
- **Strengths**: Mature, reliable, fast random access
- **Use**: All persistent data
- **Tune**: Cache size, compression, page size

### BSON
- **Strengths**: Space-efficient, binary format
- **Use**: Network transmission, storage
- **Overhead**: Minimal - efficient encoding

### simdjson
- **Strengths**: SIMD acceleration, extremely fast
- **Use**: JSON parsing in shell
- **Limitation**: Requires valid JSON input

## Troubleshooting

### WiredTiger build fails
```bash
# Usually missing Python or pkg-config
# Ubuntu/Debian
sudo apt install python3 pkg-config

# Fedora/RHEL
sudo dnf install python3 pkgconfig

# Clean and rebuild
rm -rf build && mkdir build && cd build
cmake .. && make -j$(nproc)
```

### BSON linking errors
```bash
# Ensure mongo-c-driver built correctly
# Check CMake configuration includes:
# - ENABLE_BSON=ON
# - ENABLE_MONGOC=OFF
# - ENABLE_TESTS=OFF
```

### simdjson parsing errors
```bash
# Usually invalid JSON from user/network
# Check command_parser.cpp error handling
# Verify JSON is well-formed
```

## Build Dependencies Summary

| Library | Type | Used For | Linked As |
|---------|------|----------|-----------|
| WiredTiger | External | Storage | libwiredtiger.a |
| mongo-c-driver | External | BSON | libbson.a |
| simdjson | Subdirectory | JSON | libsimdjson.a |

## See Also

- [Building](BUILDING.md) - Build instructions
- [Architecture](ARCHITECTURE.md) - System design
- [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) - Directory layout
- Official docs:
  - WiredTiger: http://source.wiredtiger.com/
  - mongo-c-driver: https://github.com/mongodb/mongo-c-driver
  - simdjson: https://simdjson.org/

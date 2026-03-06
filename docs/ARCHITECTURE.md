# AevumDB Architecture

## Overview

AevumDB is a document database with an architecture designed around simplicity, performance, and reliability. The system consists of two primary applications:

1. **aevumdb** - The server daemon that manages data persistence and query execution
2. **aevumsh** - The interactive shell client for user interaction

## Components

### Application Layer

#### aevumdb (Server Daemon)
- **Entry point**: `src/aevum/main.cpp`
- **Responsibilities**:
  - Initializes the database engine (`db::Core`)
  - Starts network server on port 55001
  - Handles POSIX signals (SIGINT, SIGTERM) for graceful shutdown
  - Coordinates all subsystems during startup and shutdown

#### aevumsh (Interactive Shell)
- **Entry point**: `src/aevum/shell/main.cpp`
- **Components**:
  - **REPL Loop** (`shell/repl/repl.hpp`): Manages the Read-Eval-Print Loop
    - Reads user input with `> ` prompt
    - Handles built-in commands: `help`, `exit`, `quit`
    - Dispatches database commands to parser
  - **Command Parser** (`shell/parser/command_parser.hpp`): Parses and executes commands
    - Extracts collection name, operation, and arguments
    - Intelligently parses nested JSON objects
    - Invokes `AevumClient` methods
    - Formats results for display

### Database Engine Layer

#### Core (`db/core/core.hpp`)
- **The heart of the database** - central orchestrator
- **Responsibilities**:
  - Owns and coordinates all subsystems:
    - `WiredTigerStore` (persistence)
    - `AuthManager` (authentication)
    - `SchemaManager` (validation)
    - `IndexManager` (query acceleration)
  - Implements public API: `insert`, `find`, `update`, `remove`, `count`, `upsert`
  - Thread-safe via `std::shared_mutex`
    - Read operations (find, count) can run concurrently
    - Write operations (insert, update, delete) are serialized

#### Storage (`db/storage/wiredtiger_store.hpp`)
- **Physical persistence layer**
- Built on WiredTiger embedded database
- Handles:
  - Connection management
  - Collection storage and retrieval
  - ACID transaction semantics
  - Compression and caching
  - Data durability

#### Authentication (`db/auth/auth_manager.hpp`)
- **User and permission management**
- Manages:
  - User accounts and API keys
  - Role-based access control (RBAC)
  - Roles: `READ_ONLY`, `READ_WRITE`, `ADMIN`
  - Permission enforcement

#### Schema Management (`db/schema/schema_manager.hpp`)
- **Optional data validation**
- Manages:
  - JSON Schema definitions per collection
  - Document validation before insertion/update
  - Schema versioning and migration

#### Indexing (`db/index/index_manager.hpp`)
- **Query acceleration**
- Manages:
  - Primary and secondary indexes
  - Compound indexes (multiple fields)
  - Index selection for query optimization

### Client Layer

#### AevumClient (`client/aevum_client.hpp`)
- **High-level C++ API**
- Provides:
  - Connection management
  - Request building and authentication
  - Database operations: insert, find, update, count, remove, upsert
  - Automatic API key injection
  - Response parsing and formatting

#### Network Connection (`client/net/connection.hpp`)
- **Low-level network communication**
- Handles:
  - TCP socket management
  - Request/response serialization
  - Wire protocol formatting
  - Error handling

#### Network Server (`client/net/server.hpp`)
- **Daemon-side network server**
- Handles:
  - TCP listening on port 55001
  - Multi-threaded request processing
  - Connection management
  - Request routing to Core

### Data Format Layer

#### BSON (`bson/doc/document.hpp`)
- **Binary JSON serialization**
- Components:
  - `Document` - In-memory representation of a document
  - `Builder` - Constructs documents programmatically
  - `Iterator` - Traverses BSON structures
  - Built on MongoDB's libbson library

#### JSON (`bson/json/json.hpp`)
- **JSON parsing and conversion**
- Uses:
  - simdjson for fast parsing
  - For shell communication and user interaction
  - Converts between JSON and BSON

### Utilities

#### Logging (`util/log/logger.hpp`)
- Structured logging with configurable levels
- Used throughout for debugging and monitoring

#### Memory Management (`util/memory/`)
- Arena allocator for efficient allocation
- Smart pointer utilities
- Memory pool management

#### String Processing (`util/string/`)
- String trimming and manipulation
- UUID generation

#### Concurrency Utilities (`util/concurrency/`)
- Thread synchronization primitives
- Spinlocks and mutexes

## Data Flow

### Write Path (Insert)

```
User Command (shell)
      ↓
REPL Loop
      ↓
Command Parser
      ↓
AevumClient.insert()
      ↓
Network Connection (JSON over TCP)
      ↓
Network Server (daemon)
      ↓
Core.insert()
      ↓
SchemaManager (validate)
      ↓
IndexManager (update indexes)
      ↓
WiredTigerStore (persist)
      ↓
Response (JSON with _id)
```

### Read Path (Find)

```
User Command (shell)
      ↓
REPL Loop
      ↓
Command Parser
      ↓
AevumClient.find()
      ↓
Network Connection (JSON over TCP)
      ↓
Network Server (daemon)
      ↓
Core.find()
      ↓
IndexManager (select best index)
      ↓
WiredTigerStore (retrieve)
      ↓
Iteration & formatting
      ↓
Response (JSON array of documents)
```

## Threading Model

### Main Daemon
- **Signal Handling Thread**: Dedicated thread for POSIX signals
  - Uses `sigwait()` for synchronous handling
  - Signals main thread to gracefully shutdown
  - Avoids reentrancy issues of async signal handlers

- **Network Server Threads**: Multi-threaded pool
  - Handles concurrent client connections
  - Each connection processed independently
  - Serializes writes via Core's `shared_mutex`

### Database Core Synchronization
- **Shared Mutex (`std::shared_mutex`)**:
  - Multiple readers can execute concurrently (find, count)
  - Exclusive write lock for modifications (insert, update, delete)
  - Prevents race conditions and ensures consistency

## Key Subsystem Interactions

### Insertion Sequence
1. Shell user types: `db.users.insert({...})`
2. Parser extracts collection name and document
3. Client sends JSON request with API key
4. Server receives and routes to Core
5. Core acquires write lock
6. SchemaManager validates if schema exists
7. UUID v4 _id generated if missing
8. IndexManager prepares index updates
9. WiredTigerStore persists document
10. Response sent back with inserted _id

### Query Sequence
1. User types: `db.users.find({age: {$gt: 25}})`
2. Parser extracts collection and query
3. Client sends JSON request
4. Server receives and routes to Core
5. Core acquires read lock
6. IndexManager selects optimal index
7. WiredTigerStore retrieves matching documents
8. Results formatted as JSON array
9. Response sent back with documents

## Configuration & Initialization

### Daemon Startup
```cpp
Core core("./data");        // Initialize engine with data directory
Server server(core);        // Create network server
server.start();             // Start listening on port 55001
// ... wait for signals ...
server.stop();              // Graceful shutdown
```

### Client Connection
```cpp
AevumClient client("127.0.0.1", 55001, "root");  // Create client
client.connect();                                  // Connect to daemon
// ... execute commands ...
client.disconnect();                              // Graceful disconnect
```

## Performance Characteristics

### Storage
- **Engine**: WiredTiger (proven, battle-tested)
- **Compression**: Available, reduces disk usage
- **Caching**: In-memory cache layer
- **Durability**: ACID transactions

### Query Processing
- **Indexing**: Automatic index selection
- **Concurrency**: Read-write lock allows parallel queries
- **Parsing**: High-speed simdjson for JSON
- **Network**: TCP/IP direct connection

### Memory
- **Arena Allocation**: Efficient bulk allocations
- **Smart Pointers**: Automatic resource management
- **Connection Pooling**: Reuse of network resources

## Security Model

### Authentication
- **API Key**: Every client provides an API key
- **Roles**: Three role levels (READ_ONLY, READ_WRITE, ADMIN)
- **User Management**: ADMIN role can create/delete users

### Authorization
- **Operation Level**: Permissions checked before execution
- **READ_ONLY**: find, count
- **READ_WRITE**: find, count, insert, update, delete, set_schema
- **ADMIN**: All operations

### Data Validation
- **Optional Schemas**: Define validation per collection
- **JSON Schema**: Standard schema validation format
- **Enforcement**: Schema checked before persistence

## Limitations & Considerations

### Single Process
- Currently single-process solution
- No built-in cluster or replication
- Good for single-node deployments

### Schema Optional
- Collections can exist without schema
- Schema validation is opt-in
- Provides flexibility at cost of safety

### No Joins
- No cross-collection joins
- Use embedding or application-level mapping
- Designed for document-oriented access patterns

### Shell Synchronous
- Shell commands wait for response
- No async/await in interactive shell
- For async, use C++ client library

## Extensibility

### Adding New Operations
1. Add method to `Core` class
2. Add handling in `AevumClient`
3. Add parser logic in `command_parser`
4. Add shell command support in REPL

### Custom Storage
- `WiredTigerStore` interface can be swapped
- Implement required interface for alternate storage

### Network Protocol
- Currently JSON over TCP
- Could extend for additional protocols
- Wire protocol defined in network layer

## See Also

- [Building](BUILDING.md) - Compile AevumDB
- [Development](DEVELOPMENT.md) - Development setup and workflow
- [Deployment](DEPLOYMENT.md) - Running in production
- [Shell Reference](SHELL_REFERENCE.md) - Complete command list
- [Project Structure](PROJECT_STRUCTURE.md) - Directory layout

# C++ Client API Reference

Use the AevumDB C++ client library to build applications that interact with AevumDB.

## Overview

The C++ client provides a high-level API for connecting to an AevumDB server and performing database operations programmatically.

**Key features**:
- Simple, modern C++ interface
- Automatic request/response handling
- JSON-based communication
- BSON document support
- Type-safe operations

## Installation

### Header-Only Include

AevumDB client is provided as a header library:

```cpp
#include "aevum/client/aevum_client.hpp"
```

Make sure to link against required libraries during build.

### Building Applications

With CMake:

```cmake
find_package(aevumdb REQUIRED)
target_link_libraries(my_app aevumdb)
```

Or manually:

```bash
g++ -std=c++17 my_app.cpp -I/path/to/aevumdb/src -L/path/to/aevumdb/build/lib -laevumdb -o my_app
```

## Quick Start

### Connect to Database

```cpp
#include <iostream>
#include "aevum/client/aevum_client.hpp"

int main() {
    // Create client
    aevum::client::AevumClient client("127.0.0.1", 55001, "root");
    
    // Connect to server
    if (!client.connect()) {
        std::cerr << "Failed to connect to server\n";
        return 1;
    }
    
    std::cout << "Connected to AevumDB!\n";
    
    // Perform operations here
    
    // Disconnect
    client.disconnect();
    
    return 0;
}
```

## Core API

### Constructor

```cpp
AevumClient(std::string host, int port, std::string api_key);
```

**Parameters**:
- `host`: Server hostname or IP (default: "127.0.0.1")
- `port`: Server port (default: 55001)
- `api_key`: Authentication key (default: "root")

**Example**:
```cpp
aevum::client::AevumClient client("localhost", 55001, "myuser");
```

### Connection

#### connect()

```cpp
[[nodiscard]] bool connect();
```

Establish connection to server. Returns `true` if successful.

```cpp
if (client.connect()) {
    // Connected successfully
}
```

#### disconnect()

```cpp
void disconnect();
```

Close connection to server.

```cpp
client.disconnect();
```

## Document Operations

### insert

Insert a new document into a collection.

```cpp
std::string insert(std::string_view collection, std::string_view document_json);
```

**Parameters**:
- `collection`: Collection name
- `document_json`: JSON string representing the document

**Returns**: JSON response with inserted `_id`

**Example**:
```cpp
std::string response = client.insert(
    "users",
    R"({"name": "Alice", "age": 30})"
);
// Response: {"status": "ok", "_id": "550e8400-e29b-41d4-a716-446655440000"}
```

### find

Query documents in a collection.

```cpp
std::string find(std::string_view collection, std::string_view query_json);
```

**Parameters**:
- `collection`: Collection name
- `query_json`: JSON query filter (use "{}" for all)

**Returns**: JSON array of matching documents

**Example**:
```cpp
// Find all users
std::string result = client.find("users", "{}");

// Find with filter
std::string result = client.find("users", R"({"age": {$gt": 25}})");

// Response: {"status": "ok", "data": [{...}, {...}]}
```

**Query Operators**:
- `$gt` - Greater than
- `$gte` - Greater than or equal
- `$lt` - Less than
- `$lte` - Less than or equal
- `$eq` - Equal
- `$ne` - Not equal
- `$in` - In array
- `$nin` - Not in array

### update

Modify existing documents.

```cpp
std::string update(
    std::string_view collection,
    std::string_view query_json,
    std::string_view update_json
);
```

**Parameters**:
- `collection`: Collection name
- `query_json`: Query to find documents
- `update_json`: New field values

**Returns**: JSON response with `updated_count`

**Example**:
```cpp
std::string response = client.update(
    "users",
    R"({"name": "Alice"})",
    R"({"age": 31})"
);
// Response: {"status": "ok", "updated_count": 1}
```

### count

Count documents matching a query.

```cpp
std::string count(std::string_view collection, std::string_view query_json);
```

**Parameters**:
- `collection`: Collection name
- `query_json`: Query filter

**Returns**: JSON response with `count`

**Example**:
```cpp
std::string response = client.count("users", R"({"age": {$gt": 25}})");
// Response: {"status": "ok", "count": 3}
```

### remove

Delete documents matching a query.

```cpp
std::string remove(std::string_view collection, std::string_view query_json);
```

**Parameters**:
- `collection`: Collection name
- `query_json`: Query to identify documents

**Returns**: JSON response with `deleted_count`

**Example**:
```cpp
std::string response = client.remove("users", R"({"age": {$lt": 18}})");
// Response: {"status": "ok", "deleted_count": 2}
```

### upsert

Update document or insert if not found.

```cpp
std::string upsert(
    std::string_view collection,
    std::string_view query_json,
    std::string_view document_json
);
```

**Example**:
```cpp
std::string response = client.upsert(
    "users",
    R"({"email": "alice@example.com"})",
    R"({"name": "Alice", "age": 30})"
);
```

## Schema Operations

### set_schema

Define schema for a collection.

```cpp
std::string set_schema(std::string_view collection, std::string_view schema_json);
```

**Example**:
```cpp
std::string schema = R"({
    "type": "object",
    "properties": {
        "name": {"type": "string"},
        "age": {"type": "number"}
    },
    "required": ["name"]
})";

client.set_schema("users", schema);
```

## User Management

### create_user

Create a new user with API key.

```cpp
std::string create_user(
    std::string_view api_key,
    std::string_view role
);
```

**Roles**:
- `READ_ONLY` - Can only read
- `READ_WRITE` - Read and write
- `ADMIN` - Full access

**Example**:
```cpp
client.create_user("myuser", "READ_WRITE");
client.create_user("analytics", "READ_ONLY");
```

## Advanced: Low-Level API

### send_request

Send raw request and receive response.

```cpp
std::string send_request(std::string_view request_json);
```

**Example**:
```cpp
std::string request = R"({
    "command": "insert",
    "collection": "users",
    "data": {"name": "Alice"}
})";
std::string response = client.send_request(request);
```

### build_payload

Build JSON payload for requests.

```cpp
std::string build_payload(
    std::string_view command,
    std::string_view collection,
    std::string_view extra
);
```

## Complete Example

```cpp
#include <iostream>
#include <string>
#include "aevum/client/aevum_client.hpp"

int main() {
    // Create client
    aevum::client::AevumClient client("127.0.0.1", 55001, "root");
    
    // Connect
    if (!client.connect()) {
        std::cerr << "Connection failed\n";
        return 1;
    }
    
    // Insert documents
    auto insert1 = client.insert("users", R"({"name": "Alice", "age": 30})");
    auto insert2 = client.insert("users", R"({"name": "Bob", "age": 25})");
    std::cout << "Inserted: " << insert1 << "\n";
    
    // Find documents
    auto find_result = client.find("users", "{}");
    std::cout << "Found: " << find_result << "\n";
    
    // Find with filter
    auto filtered = client.find("users", R"({"age": {$gt": 26}})");
    std::cout << "Filtered: " << filtered << "\n";
    
    // Update
    auto update_result = client.update(
        "users",
        R"({"name": "Alice"})",
        R"({"age": 31})"
    );
    std::cout << "Updated: " << update_result << "\n";
    
    // Count
    auto count_result = client.count("users", "{}");
    std::cout << "Count: " << count_result << "\n";
    
    // Delete
    auto delete_result = client.remove("users", R"({"age": {$lt": 26}})");
    std::cout << "Deleted: " << delete_result << "\n";
    
    // Set schema
    auto schema = client.set_schema(
        "users",
        R"({
            "type": "object",
            "properties": {
                "name": {"type": "string"},
                "age": {"type": "number"}
            }
        })"
    );
    std::cout << "Schema set: " << schema << "\n";
    
    // Create user
    auto user = client.create_user("newuser", "READ_WRITE");
    std::cout << "User created: " << user << "\n";
    
    client.disconnect();
    return 0;
}
```

## Error Handling

All API calls return JSON responses. Check the `status` field:

```cpp
std::string response = client.insert("users", R"({"name": "Alice"})");

// Parse response (simplified)
if (response.find("\"status\":\"ok\"") != std::string::npos) {
    std::cout << "Success\n";
} else if (response.find("error") != std::string::npos) {
    std::cout << "Error: " << response << "\n";
}
```

For proper JSON parsing, use a JSON library:

```cpp
#include "simdjson.h"

simdjson::dom::parser parser;
simdjson::dom::element doc = parser.parse(response);

if (doc["status"] == "ok") {
    // Handle success
    auto id = doc["_id"];
} else {
    // Handle error
    auto error = doc["message"];
}
```

## Performance Tips

1. **Reuse connections**: Keep client connected for multiple operations
2. **Batch operations**: Multiple operations in one connection
3. **Use specific queries**: `find({"_id": "..."})` faster than `find({})`
4. **Connection pooling**: Create pool for concurrent operations

Example connection pool:
```cpp
#include <vector>
#include <queue>

class ConnectionPool {
private:
    std::queue<aevum::client::AevumClient> available_;
    
public:
    ConnectionPool(size_t size) {
        for (size_t i = 0; i < size; ++i) {
            aevum::client::AevumClient client("127.0.0.1", 55001, "root");
            client.connect();
            available_.push(client);
        }
    }
    
    aevum::client::AevumClient acquire() {
        auto client = available_.front();
        available_.pop();
        return client;
    }
    
    void release(aevum::client::AevumClient client) {
        available_.push(client);
    }
};
```

## Threading Considerations

- Each thread should have its own `AevumClient` instance
- Or use a connection pool with thread-safe access
- Database operations are thread-safe on server side

## Limitations

- Synchronous operations (no async/await)
- No cursor support (all results returned at once)
- No transactions across operations
- Limited to JSON as query language

## See Also

- [Shell Reference](SHELL_REFERENCE.md) - Interactive shell commands
- [Architecture](ARCHITECTURE.md) - System design
- [Building](BUILDING.md) - Build instructions
- [Development](DEVELOPMENT.md) - Development guide
- [Project Structure](PROJECT_STRUCTURE.md) - Codebase layout

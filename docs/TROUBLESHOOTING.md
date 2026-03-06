# Troubleshooting Guide

Solutions to common problems with AevumDB.

## Build and Installation Issues

### Problem: "CMake not found"

**Error message:**
```
./scripts/build.sh: line 10: cmake: command not found
```

**Solutions:**

Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install cmake
```

macOS:
```bash
brew install cmake
```

Fedora/RHEL:
```bash
sudo dnf install cmake
```

Verify installation:
```bash
cmake --version
```

### Problem: "C++ compiler not found"

**Error message:**
```
-- The C compiler identification is unknown
-- The CXX compiler identification is unknown
```

**Solutions:**

Ubuntu/Debian:
```bash
sudo apt-get install build-essential
```

macOS:
```bash
xcode-select --install
```

Fedora/RHEL:
```bash
sudo dnf install gcc-c++ make
```

Verify installation:
```bash
gcc --version
g++ --version
```

### Problem: "WiredTiger build failed"

**Error message:**
```
ExternalProject_Add(wiredtiger_project) failed during configure
```

**Solutions:**

Clean and rebuild:
```bash
./scripts/build.sh clean
./scripts/build.sh -j 1   # Use single job for debugging
```

Check available disk space:
```bash
df -h
```

WiredTiger requires ~500MB free space during build.

### Problem: "Linking errors with undefined references"

**Error message:**
```
undefined reference to `aevum::bson::doc::Document::...`
```

**Solutions:**

Rebuild from scratch:
```bash
./scripts/build.sh clean
./scripts/build.sh
```

Check CMakeLists.txt was not corrupted:
```bash
git checkout CMakeLists.txt
./scripts/build.sh
```

## Runtime Issues

### Problem: "Port already in use"

**Error message:**
```
[ERROR] Cannot bind to port 27017: Address already in use
```

**Solutions:**

Option 1 - Use a different port:
```bash
./build/bin/aevumdb --port 27018
./build/bin/aevumsh --port 27018
```

Option 2 - Find and stop the process using the port:
```bash
lsof -i :27017
kill -9 <PID>

# Then start the server:
./build/bin/aevumdb
```

### Problem: "Permission denied" starting daemon

**Error message:**
```
./build/bin/aevumdb: Permission denied
```

**Solution:**

Make binary executable:
```bash
chmod +x ./build/bin/aevumdb
./build/bin/aevumdb
```

### Problem: "Cannot create data directory"

**Error message:**
```
[ERROR] Cannot create data directory: Permission denied
```

**Solution:**

Create with proper permissions:
```bash
mkdir -p data
chmod 755 data
./build/bin/aevumdb --data-dir ./data
```

Or for production:
```bash
sudo mkdir -p /var/lib/aevumdb/data
sudo chown $USER:$USER /var/lib/aevumdb
./build/bin/aevumdb --data-dir /var/lib/aevumdb/data
```

### Problem: "Shell cannot connect to server"

**Error message:**
```
Error: Cannot connect to localhost:27017
Connection refused
```

**Solutions:**

1. Check if server is running:
```bash
ps aux | grep aevumdb
```

2. Check if listening on correct port:
```bash
netstat -tlnp | grep 27017
```

3. Try explicit host and port:
```bash
./build/bin/aevumsh --host localhost --port 27017
```

4. Check firewall:
```bash
# Ubuntu
sudo ufw allow 27017

# Fedora
sudo firewall-cmd --permanent --add-port=27017/tcp
sudo firewall-cmd --reload
```

## Database Issues

### Problem: "Collection not found"

**Error message:**
```
Error: Collection 'users' not found in database 'myapp'
```

**Solution:**

Create the collection:
```
aevumsh> create_collection users
```

Check existing collections:
```
aevumsh> show_collections
```

### Problem: "Schema validation error"

**Error message:**
```
Error: Document does not match schema
```

**Solution:**

Check the schema:
```
aevumsh> validate_document users {name: "Alice", age: 30}
```

See which field failed validation. Fix the document or modify the schema:
```
aevumsh> drop_schema users
# Then create a new, correct schema
aevumsh> create_schema users schema.json
```

### Problem: "Duplicate key error"

**Error message:**
```
Error: Duplicate key on index 'email'
```

**Solution:**

This happens when inserting a document that violates a unique index. Either:

1. Use a different value:
```
aevumsh> insert users {name: "Bob", email: "bob@example.com"}
```

2. Or update the existing document:
```
aevumsh> update users {email: "alice@example.com"} {name: "Alice Updated"}
```

3. Or remove the unique constraint:
```
aevumsh> drop_index users email
```

## Performance Issues

### Problem: "Slow queries"

**Solutions:**

1. Create indexes on frequently queried fields:
```
aevumsh> create_index users email
aevumsh> create_index users name
```

2. Check which indexes exist:
```
aevumsh> show_indexes users
```

3. Analyze query performance:
```
aevumsh> analyze_query users {email: "alice@example.com"}
```

### Problem: "High CPU usage"

**Solutions:**

1. Check for long-running queries:
```
aevumsh> show_slow_queries
```

2. Kill problematic connections:
```
aevumsh> kill_connection <connection_id>
```

3. Add indexes to speed up queries

### Problem: "Out of memory"

**Solutions:**

1. Check memory usage:
```bash
free -h
ps aux | grep aevumdb
```

2. Reduce cache size in config:
```ini
[storage]
cache_size = 4GB  # Reduce from default
```

3. Close idle connections:
```
aevumsh> show_connections
aevumsh> kill_connection <id>  # For idle connections
```

## Debug Mode

### Enable verbose logging

```bash
./build/bin/aevumdb --log-level debug
```

Check logs:
```bash
tail -f data/logs/aevumdb.log
```

### Run with GDB debugger

```bash
gdb ./build/bin/aevumdb
(gdb) run --log-level debug
(gdb) bt  # Print backtrace on crash
```

### Collect diagnostic information

```bash
aevumsh> dump diagnostics to "diag-$(date +%Y%m%d-%H%M%S).tar.gz"
```

Send this file when reporting issues.

## Getting More Help

1. Check the [API Reference](API.md) for command documentation
2. See [Deployment Guide](DEPLOYMENT.md#troubleshooting) for production issues
3. Check [Development Guide](DEVELOPMENT.md#troubleshooting-development) for development issues
4. Search [GitHub Issues](https://github.com/aevumdb/aevum/issues)
5. Ask on [GitHub Discussions](https://github.com/aevumdb/aevum/discussions)

## Reporting a Bug

When reporting an issue, include:

1. **Version:**
```bash
./build/bin/aevumdb --version
```

2. **OS and hardware:**
```bash
uname -a
free -h
```

3. **Steps to reproduce** - Exact commands that cause the issue

4. **Error output:**
```bash
./build/bin/aevumdb --log-level debug 2>&1 | tee error.log
```

5. **Diagnostic dump:**
```
aevumsh> dump diagnostics to "diag.tar.gz"
```

Upload these to your GitHub issue.

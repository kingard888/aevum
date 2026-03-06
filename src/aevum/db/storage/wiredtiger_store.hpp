// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file wiredtiger_store.hpp
 * @brief Defines the `WiredTigerStore` class, a high-level C++ abstraction over the
 * WiredTiger embedded storage engine.
 * @details This header declares the primary interface for all persistent storage operations in
 * AevumDB. The `WiredTigerStore` class encapsulates the low-level C API of WiredTiger, providing
 * a robust, object-oriented mechanism for database initialization, collection (table) management,
 * and CRUD operations on BSON documents.
 */
#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef HAVE_WIREDTIGER
#include <wiredtiger.h>
#else
// Minimal stubs when WiredTiger is not available.
typedef struct __wt_connection WT_CONNECTION;
typedef struct __wt_session WT_SESSION;
typedef struct __wt_cursor WT_CURSOR;
#endif

#include "aevum/bson/doc/document.hpp"
#include "aevum/util/status.hpp"

namespace aevum::db::storage {

/**
 * @class WiredTigerStore
 * @brief Provides a modern C++ facade for interacting with the WiredTiger key-value store.
 *
 * @details This class is the foundational persistence layer of AevumDB. It manages the complete
 * lifecycle of a WiredTiger database instance, from initialization (`init`) to shutdown
 * (`~WiredTigerStore`). It translates high-level database concepts like "collections" and
 * "documents" into WiredTiger's native table and key-value pair semantics. All public methods
 * provide a clean, status-based error handling mechanism and are designed to work seamlessly with
 * the `aevum::bson::doc::Document` type for storing BSON data.
 */
class WiredTigerStore {
  public:
    /**
     * @brief Constructs a `WiredTigerStore` instance associated with a specific filesystem path.
     * @param base_path The root directory path where WiredTiger will create and manage its data
     * files. This path will be created if it does not already exist.
     */
    explicit WiredTigerStore(std::string base_path);

    /**
     * @brief Destroys the `WiredTigerStore`, ensuring a graceful shutdown of the underlying
     * WiredTiger connection.
     * @details This is a critical step to prevent data corruption. It explicitly closes the
     * `WT_CONNECTION` if it is open, flushing all changes to disk.
     */
    ~WiredTigerStore();

    // The WiredTigerStore is non-copyable and non-movable to enforce a single point of control
    // over the physical database files and the active database connection.
    WiredTigerStore(const WiredTigerStore &) = delete;
    WiredTigerStore &operator=(const WiredTigerStore &) = delete;
    WiredTigerStore(WiredTigerStore &&) = delete;
    WiredTigerStore &operator=(WiredTigerStore &&) = delete;

    /**
     * @brief Initializes the WiredTiger storage engine and opens the database connection.
     * @details This method must be called before any other operations can be performed. It ensures
     * the base data directory exists and then calls `wiredtiger_open` to either create a new
     * database at that path or open an existing one. It also configures essential parameters
     * like cache size.
     * @return `aevum::util::Status::OK()` on successful initialization.
     * @return `aevum::util::Status::IOError` if the directory cannot be created or if
     *         `wiredtiger_open` fails.
     */
    [[nodiscard]] aevum::util::Status init();

    /**
     * @brief Retrieves a list of all user-defined collection names from the database.
     * @details This function queries the special `metadata:` table within WiredTiger to discover
     * all user-created tables, filtering out internal WiredTiger tables.
     * @return A `std::vector<std::string>` containing the names of all collections.
     */
    [[nodiscard]] std::vector<std::string> list_collections();

    /**
     * @brief Loads and deserializes all documents from a specified collection.
     * @details This function opens a cursor on the collection's table and iterates through every
     * key-value pair, deserializing the raw binary value of each record back into a
     * `aevum::bson::doc::Document` object.
     * @param collection The name of the collection to load.
     * @return A `std::vector` of all `Document`s in the collection. Returns an empty vector if the
     *         collection is empty or if an error occurs during the process.
     */
    [[nodiscard]] std::vector<aevum::bson::doc::Document> load_collection(
        std::string_view collection);

    /**
     * @brief Inserts a new document or updates an existing one in a collection.
     * @details This function performs an "upsert" operation. It uses the provided `id` as the key.
     * If a record with this key already exists, its value is overwritten with the new BSON data.
     * If the key does not exist, a new record is created.
     * @param collection The target collection.
     * @param id The unique identifier for the document, used as the primary key in storage.
     * @param doc The `aevum::bson::doc::Document` to be stored.
     * @return `aevum::util::Status::OK()` on success, or an `IOError` if the write operation fails.
     */
    [[nodiscard]] aevum::util::Status put(std::string_view collection, std::string_view id,
                                          const aevum::bson::doc::Document &doc);

    /**
     * @brief Removes a document from a collection using its unique identifier.
     * @param collection The target collection.
     * @param id The unique ID of the document to be removed.
     * @return `aevum::util::Status::OK()` on successful removal or if the key was not found.
     *         Returns an `IOError` if the removal operation fails for other reasons.
     */
    [[nodiscard]] aevum::util::Status remove(std::string_view collection, std::string_view id);

    /**
     * @brief Atomically replaces the entire content of a collection with a new set of documents.
     * @details This powerful operation first drops the existing table for the collection and then
     * re-creates it. It subsequently performs a bulk insert of all the provided documents. This is
     * highly efficient for performing wholesale updates on a collection's data, as returned by
     * the Rust FFI update/delete operations.
     * @param collection The target collection to be synchronized.
     * @param documents A vector of `std::pair<std::string, Document>` representing the complete
     *        new content for the collection, where the first element is the document ID (key) and
     *        the second is the document itself (value).
     * @return `aevum::util::Status::OK()` on success, or an `IOError` on failure.
     */
    [[nodiscard]] aevum::util::Status sync_collection(
        std::string_view collection,
        const std::vector<std::pair<std::string, aevum::bson::doc::Document>> &documents);

    /**
     * @brief Provides direct, low-level access to the raw WiredTiger connection pointer.
     * @details This is an escape hatch for advanced use cases that may require direct interaction
     * with the WiredTiger C API, such as managing sessions or cursors outside the scope of this
     * class's provided methods. Use with caution.
     * @return A pointer to the underlying `WT_CONNECTION` object.
     */
    [[nodiscard]] WT_CONNECTION *connection() const { return conn_; }

  private:
    /// The root filesystem path for the WiredTiger database files.
    std::string base_path_;
    /// A pointer to the active WiredTiger database connection instance.
    WT_CONNECTION *conn_{nullptr};

    /**
     * @brief A private utility to ensure that a WiredTiger table for a given collection exists.
     * @details If the table does not already exist, it is created with a default configuration
     * (`key_format=S` for string keys, `value_format=u` for raw BSON byte arrays).
     * @param session The active WiredTiger session in which to perform the operation.
     * @param collection The name of the collection (table) to check for existence.
     * @return `aevum::util::Status::OK()` on success or if the table already exists.
     *         Returns an `IOError` if table creation fails.
     */
    [[nodiscard]] aevum::util::Status ensure_table(WT_SESSION *session,
                                                   std::string_view collection);

    /**
     * @brief A private helper to construct the standard WiredTiger URI for a table.
     * @param collection The name of the collection.
     * @return The formatted WiredTiger URI string (e.g., "table:my_collection").
     */
    [[nodiscard]] std::string make_uri(std::string_view collection) const;
};

}  // namespace aevum::db::storage

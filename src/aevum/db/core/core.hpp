// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file core.hpp
 * @brief Declares the `Core` class, the central nervous system of the AevumDB database engine.
 * @details This header file defines the primary C++ class that orchestrates all database
 * subsystems. The `Core` class integrates storage, authentication, indexing, and schema
 * management into a cohesive, thread-safe public API for all high-level database operations.
 */
#pragma once

#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>

#include "aevum/bson/doc/document.hpp"
#include "aevum/db/auth/auth_manager.hpp"
#include "aevum/db/index/index_manager.hpp"
#include "aevum/db/schema/schema_manager.hpp"
#include "aevum/db/storage/wiredtiger_store.hpp"
#include "aevum/util/status.hpp"

/**
 * @namespace aevum::db
 * @brief Contains the primary components and subsystems that constitute the AevumDB database
 * engine.
 */
namespace aevum::db {

/**
 * @class Core
 * @brief The central, coordinating class for the AevumDB engine, providing the main public API.
 *
 * @details The `Core` class is the heart of the database, responsible for initializing, managing,
 * and wiring together all critical subsystems. It owns the instances of `WiredTigerStore`,
 * `AuthManager`, `SchemaManager`, and `IndexManager`. It exposes a comprehensive public API for
 * all database operations (e.g., `insert`, `find`, `update`, `remove`), acting as a facade that
 * routes requests to the appropriate subsystem or to the Rust FFI query engine.
 *
 * Thread safety for all database-wide operations is ensured at this level via a
 * `std::shared_mutex`, allowing for concurrent read operations (`find`, `count`) while serializing
 * write operations
 * (`insert`, `update`, etc.) to maintain data consistency across all subsystems.
 */
class Core {
  public:
    /**
     * @brief Constructs and initializes the entire AevumDB engine.
     * @details The constructor orchestrates the complete startup sequence: it initializes the
     * `WiredTigerStore` persistence layer, creates the manager instances for auth, schema, and
     * indexing, and then calls `load_all()` to populate these managers with data from disk. If no
     * users are found, it also bootstraps a default 'root' administrator.
     * @param data_dir The filesystem path where the `WiredTigerStore` will persist its data files.
     */
    explicit Core(std::string data_dir);

    /**
     * @brief Destroys the `Core` engine, ensuring a graceful shutdown.
     * @details The destructor is responsible for ensuring that all resources, particularly the
     * underlying `WiredTigerStore` connection, are cleanly released.
     */
    ~Core();

    // The Core class is non-copyable and non-movable to enforce a singleton-like control
    // over the database instance and its underlying physical resources.
    Core(const Core &) = delete;
    Core &operator=(const Core &) = delete;
    Core(Core &&) = delete;
    Core &operator=(Core &&) = delete;

    /**
     * @brief Inserts a single document into a collection.
     * @details This is a write-locked operation. It validates the document against the collection's
     * schema, automatically generates a UUIDv4 `_id` if one is not present, persists the document
     * to storage, and finally updates all relevant indexes.
     * @param coll The name of the target collection.
     * @param doc The `aevum::bson::doc::Document` to insert. Ownership is taken by the function.
     * @return A `std::pair` where the first element is the `aevum::util::Status` of the operation
     * and the second is the `std::string` `_id` of the inserted document.
     */
    std::pair<aevum::util::Status, std::string> insert(std::string_view coll,
                                                       aevum::bson::doc::Document doc);

    /**
     * @brief Updates an existing document or inserts it if it does not exist (an "upsert"
     * operation).
     * @details This function first performs a `count` to determine if any documents match the
     * query. If one or more matches exist, it delegates to `update`. Otherwise, it delegates to
     * `insert`.
     * @param coll The name of the collection.
     * @param query_json A JSON string query to find the document to upsert.
     * @param doc The document data to use for the update or insertion.
     * @return An `aevum::util::Status` indicating the outcome of the operation.
     */
    aevum::util::Status upsert(std::string_view coll, std::string_view query_json,
                               const aevum::bson::doc::Document &doc);

    /**
     * @brief Counts the number of documents in a collection that match a query.
     * @details This is a read-locked operation that delegates its logic to the Rust FFI for high
     * performance.
     * @param coll The name of the collection.
     * @param query_json A JSON string defining the filter criteria.
     * @return The integer count of matching documents.
     */
    [[nodiscard]] int count(std::string_view coll, std::string_view query_json);

    /**
     * @brief Finds and retrieves documents from a collection matching a complex query.
     * @details This is a read-locked operation. It first checks for a simple `_id`-only query,
     * which can be served with high performance by the primary index. For all other complex
     * queries, it fetches the entire collection and delegates the filtering, sorting, projection,
     * and pagination to the high-performance Rust FFI query engine.
     * @param coll The name of the collection.
     * @param query_json A JSON string for the filter conditions.
     * @param sort_json A JSON string for the sort order.
     * @param projection_json A JSON string for field projection.
     * @param limit The maximum number of documents to return.
     * @param skip The number of initial documents to skip.
     * @return A `std::vector` of `aevum::bson::doc::Document`s that match the query criteria.
     */
    [[nodiscard]] std::vector<aevum::bson::doc::Document> find(
        std::string_view coll, std::string_view query_json, std::string_view sort_json = "{}",
        std::string_view projection_json = "{}", int64_t limit = 0, int64_t skip = 0);

    /**
     * @brief Updates all documents in a collection that match a query.
     * @details This is a write-locked operation. It delegates the update logic to the Rust FFI,
     * receives the entire modified collection back, and then atomically synchronizes this new
     * dataset to the `WiredTigerStore`, subsequently rebuilding the collection's indexes.
     * @param coll The name of the collection.
     * @param query_json A JSON query to select documents to update.
     * @param update_json A JSON document describing the modifications.
     * @return A `std::pair` containing the operation `Status` and the integer count of affected
     * documents.
     */
    std::pair<aevum::util::Status, int> update(std::string_view coll, std::string_view query_json,
                                               std::string_view update_json);

    /**
     * @brief Removes documents from a collection matching a query.
     * @details This is a write-locked operation. It first uses the `find` FFI to get the `_id`s of
     * all matching documents. It then iterates through these IDs, removing each document from the
     * `WiredTigerStore` and all associated indexes.
     * @param coll The name of the collection.
     * @param query_json A JSON query to select documents for removal.
     * @return A `std::pair` containing the operation `Status` and the integer count of removed
     * documents.
     */
    std::pair<aevum::util::Status, int> remove(std::string_view coll, std::string_view query_json);

    /**
     * @brief Sets or updates the schema for a collection.
     * @param coll The name of the collection.
     * @param schema The BSON document defining the schema.
     * @return A `aevum::util::Status` indicating the outcome.
     */
    aevum::util::Status set_schema(std::string_view coll, const aevum::bson::doc::Document &schema);

    /**
     * @brief Creates a new secondary index on a field.
     * @param coll The name of the collection.
     * @param field The field on which to create the index.
     * @return A `aevum::util::Status` indicating the outcome.
     */
    aevum::util::Status create_index(std::string_view coll, std::string_view field);

    /**
     * @brief Creates a new user and persists their credentials.
     * @param raw_key The plain-text API key for the new user.
     * @param role The user's assigned `UserRole`.
     * @return A `aevum::util::Status` indicating the outcome.
     */
    aevum::util::Status create_user(std::string_view raw_key, auth::UserRole role);

    /**
     * @brief Authenticates a user's raw API key.
     * @param raw_key The plain-text API key.
     * @return The authenticated `UserRole`, or `UserRole::NONE` on failure.
     */
    [[nodiscard]] auth::UserRole authenticate(std::string_view raw_key) const;

  private:
    /// Manages the physical storage layer via WiredTiger.
    storage::WiredTigerStore storage_;
    /// Manages in-memory authentication credential caching.
    auth::AuthManager auth_manager_;
    /// Manages schema caching, persistence, and validation.
    schema::SchemaManager schema_manager_;
    /// Orchestrates primary and secondary document indexing.
    index::IndexManager index_manager_;

    /**
     * @var rw_lock_
     * @brief A top-level reader-writer mutex that provides thread-safe access to the entire
     * database state, synchronizing all public API operations.
     */
    mutable std::shared_mutex rw_lock_;

    /**
     * @brief A private helper called during construction to load all persisted data.
     * @details This function loads all collections, schemas, indexes, and user authentication
     * data from the `WiredTigerStore` into the respective in-memory manager components.
     */
    void load_all();

    /**
     * @brief Retrieves all documents from a collection and serializes them into a single JSON array
     * string.
     * @details This is a utility function used to prepare the entire dataset for processing by the
     * Rust FFI.
     * @param coll The name of the collection to fetch.
     * @return A `std::string` containing the JSON array representation of the collection.
     */
    [[nodiscard]] std::string fetch_collection_as_json_array(std::string_view coll);
};

}  // namespace aevum::db

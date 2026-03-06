// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file schema_manager.hpp
 * @brief Defines the `SchemaManager` class, a thread-safe component for managing the lifecycle
 * of collection schemas.
 * @details This header declares the class responsible for caching, persisting, and dispatching
 * validation for database collection schemas. It acts as the primary interface for all
 * schema-related operations within the database core, ensuring that data integrity is maintained
 * according to user-defined rules.
 */
#pragma once

#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "aevum/bson/doc/document.hpp"
#include "aevum/db/storage/wiredtiger_store.hpp"
#include "aevum/util/status.hpp"

namespace aevum::db::schema {

/**
 * @class SchemaManager
 * @brief Orchestrates schema caching, persistence, and validation for all database collections.
 *
 * @details The `SchemaManager` maintains an in-memory, thread-safe cache of BSON schemas, mapping
 * each collection name to its corresponding validation rules. It uses a `std::shared_mutex` to
 * allow for highly concurrent read access (i.e., multiple simultaneous `validate` calls) while
 * ensuring that write operations (e.g., `set_schema`, `add_schema_to_cache`) are atomic and
 * exclusive.
 *
 * This class is also responsible for the entire lifecycle of a schema, from its definition and
 * in-memory caching to its persistence in a dedicated `_schemas` system collection within the
 * `WiredTigerStore`.
 */
class SchemaManager {
  public:
    /**
     * @brief Constructs a `SchemaManager`, linking it to the persistent storage layer.
     * @param storage A reference to the active `WiredTigerStore` instance, which will be used
     *        for all schema persistence operations.
     */
    explicit SchemaManager(aevum::db::storage::WiredTigerStore &storage);

    /**
     * @brief Default destructor.
     */
    ~SchemaManager() = default;

    // The SchemaManager is non-copyable and non-movable to ensure a single, consistent
    // source of truth for all schema definitions within the database.
    SchemaManager(const SchemaManager &) = delete;
    SchemaManager &operator=(const SchemaManager &) = delete;
    SchemaManager(SchemaManager &&) = delete;
    SchemaManager &operator=(SchemaManager &&) = delete;

    /**
     * @brief Validates a given BSON document against the registered schema for its collection.
     * @details This function performs a highly concurrent, read-locked lookup for the collection's
     * schema in the in-memory cache. If a schema is found, validation is dispatched to the
     * `validate_via_rust` function. If no schema is registered for the collection, the document
     * is considered trivially valid, and the operation succeeds.
     *
     * @param collection The name of the collection to which the document belongs.
     * @param doc The `aevum::bson::doc::Document` to be validated.
     * @return `aevum::util::Status::OK()` if the document is valid or if no schema is defined
     *         for the collection.
     * @return `aevum::util::Status::InvalidArgument` if the document fails to conform to the
     *         registered schema.
     */
    [[nodiscard]] aevum::util::Status validate(std::string_view collection,
                                               const aevum::bson::doc::Document &doc) const;

    /**
     * @brief Defines or updates the schema for a specific collection and persists this change.
     * @details This is a write-locked operation that first persists the provided schema document
     * to the `_schemas` system collection in the `WiredTigerStore`. Upon successful persistence,
     * it then updates the in-memory cache with the new schema definition.
     *
     * @param collection The name of the collection whose schema is being set or updated.
     * @param schema The `aevum::bson::doc::Document` that defines the new validation rules.
     * @return `aevum::util::Status::OK()` on successful persistence and cache update. Returns an
     *         `IOError` or other error status if the underlying storage operation fails.
     */
    [[nodiscard]] aevum::util::Status set_schema(std::string_view collection,
                                                 const aevum::bson::doc::Document &schema);

    /**
     * @brief Retrieves the schema registered for a specific collection.
     * @param collection The name of the collection.
     * @return An std::optional containing a copy of the schema document if found, otherwise
     * std::nullopt.
     */
    [[nodiscard]] std::optional<aevum::bson::doc::Document> get_schema(
        std::string_view collection) const;

    /**
     * @brief Adds a schema document to the in-memory cache.
     * @details This function is primarily intended for internal use during the database startup
     * sequence to populate the cache from schemas loaded from the `_schemas` collection. It
     * acquires an exclusive write lock to ensure the cache is updated atomically.
     *
     * @param collection The name of the collection, which will be moved into the cache map.
     * @param schema The BSON schema document, which will be moved into the cache map.
     */
    void add_schema_to_cache(std::string collection, aevum::bson::doc::Document schema);

  private:
    /**
     * @var storage_
     * @brief A reference to the underlying persistent storage engine, used for all schema
     * read/write operations.
     */
    aevum::db::storage::WiredTigerStore &storage_;

    /**
     * @var schemas_
     * @brief The in-memory cache for collection schemas. This hash map provides fast lookups
     * from a collection name (`std::string`) to its corresponding BSON schema `Document`.
     */
    std::unordered_map<std::string, aevum::bson::doc::Document> schemas_;

    /**
     * @var schemas_lock_
     * @brief A mutable reader-writer mutex that guarantees thread-safe access to the `schemas_`
     * cache. It allows for concurrent `validate` operations while serializing `set_schema`
     * and `add_schema_to_cache` operations.
     */
    mutable std::shared_mutex schemas_lock_;
};

}  // namespace aevum::db::schema

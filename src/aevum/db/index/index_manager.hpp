// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file index_manager.hpp
 * @brief Defines the `IndexManager` class, the central orchestrator for all document indexing
 * operations.
 * @details This header declares the high-level `IndexManager`, which serves as a unified facade
 * for interacting with both primary (`_id`-based) and secondary (field-based) indexes. It
 * coordinates the `PrimaryIndexer`, `SecondaryIndexer`, and `IndexPersistor` sub-components to
 * provide a cohesive and thread-safe indexing subsystem.
 */
#pragma once

#include "aevum/bson/doc/document.hpp"
#include "aevum/db/storage/wiredtiger_store.hpp"
#include "aevum/util/status.hpp"

// Forward declarations for the constituent sub-components of the indexing system.
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>

#include "aevum/db/index/index_persistor.hpp"
#include "aevum/db/index/primary_indexer.hpp"
#include "aevum/db/index/secondary_indexer.hpp"

namespace aevum::db::index {

/**
 * @class IndexManager
 * @brief A high-level coordinator for primary and secondary indexing services, ensuring data
 * consistency and optimized query performance.
 *
 * @details The `IndexManager` acts as the primary entry point for all indexing-related tasks within
 * the database core. It abstracts the complexities of managing distinct primary and secondary
 * index structures by delegating tasks to its specialized sub-components. All public methods are
 * designed to be thread-safe, using a `std::shared_mutex` to manage concurrent read and write
 * access to the underlying index data structures.
 */
class IndexManager {
  public:
    /**
     * @brief Constructs an `IndexManager` instance, initializing its sub-components.
     * @param storage A reference to the `WiredTigerStore`, which is passed to the `IndexPersistor`
     *        to handle the durable storage of index metadata.
     */
    explicit IndexManager(aevum::db::storage::WiredTigerStore &storage);

    /**
     * @brief Default destructor.
     */
    ~IndexManager() = default;

    // The IndexManager is non-copyable and non-movable to ensure that it maintains singular
    // and unambiguous control over the in-memory index structures and their state.
    IndexManager(const IndexManager &) = delete;
    IndexManager &operator=(const IndexManager &) = delete;
    IndexManager(IndexManager &&) = delete;
    IndexManager &operator=(IndexManager &&) = delete;

    /**
     * @brief Performs a full rebuild of all indexes for a specific collection.
     * @details This is a write-intensive operation that acquires an exclusive lock. It first clears
     * all existing primary and secondary index data for the collection and then iterates through
     * the provided documents to repopulate the indexes from scratch.
     * @param collection The name of the collection whose indexes are to be rebuilt.
     * @param documents A vector of all BSON documents currently in the collection.
     */
    void rebuild_index(std::string_view collection,
                       const std::vector<aevum::bson::doc::Document> &documents);

    /**
     * @brief Creates a new secondary index on a specified field and persists the definition.
     * @details This method first checks if the index already exists. If not, it registers the new
     * field for indexing, triggers a full `rebuild_index` to backfill existing data, and finally
     * calls `persist_index_definitions` to save the new configuration to storage.
     * @param collection The name of the target collection.
     * @param field The document field on which to create the new index.
     * @param existing_documents A comprehensive list of documents already in the collection,
     *        required for backfilling the new index.
     * @return An `aevum::util::Status` indicating the outcome of the persistence operation.
     */
    [[nodiscard]] aevum::util::Status create_index(
        std::string_view collection, std::string_view field,
        const std::vector<aevum::bson::doc::Document> &existing_documents);

    /**
     * @brief Retrieves a document directly from the primary index using its unique `_id`.
     * @details This method provides a highly optimized path for direct lookups, delegating the
     * call to the `PrimaryIndexer`. It acquires a shared read lock.
     * @param collection The name of the collection.
     * @param id The unique `_id` of the document to retrieve.
     * @return An `std::optional` containing the `Document` if found, or `std::nullopt`.
     */
    [[nodiscard]] std::optional<aevum::bson::doc::Document> get_document_by_id(
        std::string_view collection, std::string_view id) const;

    /**
     * @brief Retrieves all documents in a collection from the in-memory primary index.
     * @param collection The name of the collection.
     * @return A vector of BSON documents.
     */
    [[nodiscard]] std::vector<aevum::bson::doc::Document> get_all_documents(
        std::string_view collection) const;

    /**
     * @brief Retrieves all documents matching a specific value in a secondary index.
     * @details Delegates to the `SecondaryIndexer` to perform an efficient, index-backed query.
     * This operation acquires a shared read lock.
     * @param collection The name of the collection.
     * @param field The indexed field to query.
     * @param value The value to match within the indexed field.
     * @return A `std::vector` of matching `Document`s. Returns an empty vector if no matches are
     * found.
     */
    [[nodiscard]] std::vector<aevum::bson::doc::Document> get_documents_by_secondary_index(
        std::string_view collection, std::string_view field, std::string_view value) const;

    /**
     * @brief Atomically adds a new document to both the primary and all applicable secondary
     * indexes.
     * @details This is a write operation that should be invoked after a document has been
     * successfully inserted into the storage engine.
     * @param collection The name of the collection where the document was inserted.
     * @param doc The BSON document to be added to the indexes.
     */
    void add_document_to_indexes(std::string_view collection,
                                 const aevum::bson::doc::Document &doc);

    /**
     * @brief Atomically removes a document from both the primary and all applicable secondary
     * indexes.
     * @details This is a write operation that should be invoked after a document has been
     * successfully removed from the storage engine.
     * @param collection The name of the collection from which the document was removed.
     * @param doc The BSON document to be removed from the indexes.
     */
    void remove_document_from_indexes(std::string_view collection,
                                      const aevum::bson::doc::Document &doc);

    /**
     * @brief Loads all secondary index definitions from persistent storage into memory.
     * @details This method is a key part of the database startup sequence. It acquires an exclusive
     * lock and delegates the loading process to the `IndexPersistor`.
     * @return An `aevum::util::Status` indicating the success or failure of the load operation.
     */
    [[nodiscard]] aevum::util::Status load_all_index_definitions();

  private:
    /// An instance of the primary indexer, responsible for `_id`-based lookups.
    PrimaryIndexer primary_indexer_;
    /// An instance of the secondary indexer, responsible for field-based lookups.
    SecondaryIndexer secondary_indexer_;
    /// A component responsible for reading and writing index metadata to durable storage.
    IndexPersistor index_persistor_;

    /**
     * @var rw_lock_
     * @brief A reader-writer mutex that provides overarching, thread-safe access to all index
     * structures managed by this class. It ensures that complex operations like `rebuild_index`
     * are fully atomic with respect to queries.
     */
    mutable std::shared_mutex rw_lock_;

    /**
     * @brief Persists the current configuration of secondary indexes to durable storage.
     * @details This private helper acquires an exclusive lock and delegates the serialization
     * and storage task to the `IndexPersistor`.
     * @return An `aevum::util::Status` indicating the outcome of the persistence operation.
     */
    [[nodiscard]] aevum::util::Status persist_index_definitions();

    /**
     * @brief A private helper to efficiently extract the string representation of a document's
     * `_id`.
     * @param doc The BSON document from which to extract the `_id`.
     * @return The `_id` as a `std::string`, or an empty string if the `_id` is not found or is not
     * a string.
     */
    [[nodiscard]] std::string extract_id(const aevum::bson::doc::Document &doc) const;
};

}  // namespace aevum::db::index

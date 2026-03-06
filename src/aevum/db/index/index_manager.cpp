// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file index_manager.cpp
 * @brief Implements the `IndexManager`, the high-level coordinator for all database indexing
 * services.
 * @details This file provides the concrete implementations for the `IndexManager`'s methods.
 * It orchestrates the actions of the `PrimaryIndexer`, `SecondaryIndexer`, and `IndexPersistor`
 * to present a unified, thread-safe interface for managing and utilizing document indexes.
 */
#include "aevum/db/index/index_manager.hpp"

#include <bson/bson.h>
#include <mutex>

#include "aevum/util/log/logger.hpp"

namespace aevum::db::index {

/**
 * @brief Constructs an `IndexManager`, initializing its subsidiary components.
 * @param storage A reference to the `WiredTigerStore`, which is passed directly to the
 *        `IndexPersistor` to enable it to read and write index definitions.
 */
IndexManager::IndexManager(aevum::db::storage::WiredTigerStore &storage)
    : index_persistor_(storage) {}

/**
 * @brief A robust helper function to extract the `_id` field from a BSON document as a string.
 * @details This utility safely inspects a BSON document for a UTF-8 encoded `_id` field. It is
 * used throughout the `IndexManager` to obtain the primary key required for index operations.
 * @param doc The `aevum::bson::doc::Document` to inspect.
 * @return A `std::string` containing the `_id`. Returns an empty string if the document is null,
 *         empty, or if the `_id` field is missing or not a valid UTF-8 string.
 */
std::string IndexManager::extract_id(const aevum::bson::doc::Document &doc) const {
    if (doc.empty() || !doc.get()) return "";

    bson_iter_t iter;
    if (bson_iter_init_find(&iter, doc.get(), "_id") && BSON_ITER_HOLDS_UTF8(&iter)) {
        uint32_t length;
        const char *id_str = bson_iter_utf8(&iter, &length);
        return std::string(id_str, length);
    }
    return "";
}

/**
 * @brief Performs an atomic, full rebuild of all indexes for a given collection.
 * @details This is a heavyweight, write-locked operation. It first acquires an exclusive lock on
 * the `rw_lock_` to prevent any other read or write operations on the indexes. It then delegates
 * to the `PrimaryIndexer` and `SecondaryIndexer` to clear all of their existing index data for
 * the specified collection. Finally, it iterates through the provided `documents` and adds each
 * one back into the primary and secondary indexes, effectively repopulating them from scratch.
 *
 * @param collection The name of the collection whose indexes are being rebuilt.
 * @param documents A vector containing all documents currently in the collection.
 */
void IndexManager::rebuild_index(std::string_view collection,
                                 const std::vector<aevum::bson::doc::Document> &documents) {
    std::string coll_str(collection);
    aevum::util::log::Logger::debug("IndexManager: Starting full index rebuild for collection '" +
                                    coll_str + "'.");
    // Acquire an exclusive write lock to ensure the entire rebuild process is atomic.
    std::unique_lock<std::shared_mutex> lock(rw_lock_);

    primary_indexer_.clear_collection_index(coll_str);
    secondary_indexer_.clear_collection_indexes(coll_str);
    aevum::util::log::Logger::debug(
        "IndexManager: Cleared all existing primary and secondary index data for '" + coll_str +
        "'.");

    for (const auto &doc : documents) {
        std::string id = extract_id(doc);
        if (!id.empty()) {
            primary_indexer_.add_document_to_primary_index(coll_str, id, doc);
        }
        // This will update all relevant secondary indexes for the document.
        secondary_indexer_.update_custom_index(coll_str, doc, true);
    }
    aevum::util::log::Logger::info(
        "IndexManager: Successfully rebuilt all indexes for collection '" + coll_str + "' with " +
        std::to_string(documents.size()) + " documents.");
}

/**
 * @brief Creates a new secondary index on a field, backfills it with existing data, and persists
 * the definition.
 * @details This method orchestrates the entire process of introducing a new secondary index. It
 * first acquires an exclusive lock to check if the index already exists; if so, it returns
 * immediately. If not, it registers the new field with the `SecondaryIndexer`. It then triggers a
 * full `rebuild_index` to populate the new index with all existing documents. Finally, it calls
 * `persist_index_definitions` to ensure the new index configuration is saved durably.
 *
 * @param collection The name of the collection on which to create the index.
 * @param field The name of the field to be indexed.
 * @param existing_documents A vector of all documents currently in the collection, required for
 * backfilling.
 * @return `aevum::util::Status::OK()` on success. Returns an error status if the final persistence
 * step fails.
 */
aevum::util::Status IndexManager::create_index(
    std::string_view collection, std::string_view field,
    const std::vector<aevum::bson::doc::Document> &existing_documents) {
    std::string coll_str(collection);
    std::string field_str(field);
    aevum::util::log::Logger::debug("IndexManager: Request to create index on '" + coll_str + "." +
                                    field_str + "'.");

    {
        // Scope for a write lock to modify the index field definitions.
        std::unique_lock<std::shared_mutex> lock(rw_lock_);
        if (secondary_indexer_.is_field_indexed(coll_str, field_str)) {
            aevum::util::log::Logger::warn("IndexManager: Index on '" + coll_str + "." + field_str +
                                           "' already exists. No action taken.");
            return aevum::util::Status::OK();  // Index already exists, operation is idempotent.
        }
        secondary_indexer_.add_indexed_field(coll_str, field_str);
        aevum::util::log::Logger::info("IndexManager: Registered new secondary index for '" +
                                       coll_str + "." + field_str + "'.");
    }

    // Rebuild indexes to backfill the newly created index with existing data.
    rebuild_index(collection, existing_documents);

    // Persist the updated index definitions to durable storage.
    return persist_index_definitions();
}

/**
 * @brief Retrieves a document directly by its primary key (`_id`).
 * @details This is a high-performance query path that delegates directly to the `PrimaryIndexer`.
 * It acquires a shared read lock, allowing for high-concurrency lookups.
 * @param collection The name of the collection.
 * @param id The unique `_id` of the document to retrieve.
 * @return An `std::optional` containing the document if found, otherwise `std::nullopt`.
 */
std::optional<aevum::bson::doc::Document> IndexManager::get_document_by_id(
    std::string_view collection, std::string_view id) const {
    // Acquire a shared (read) lock to allow concurrent lookups.
    std::shared_lock<std::shared_mutex> lock(rw_lock_);
    return primary_indexer_.get_document_by_id(std::string(collection), std::string(id));
}

/**
 * @brief Retrieves all documents in a collection from the in-memory primary index.
 * @details This method provides a high-concurrency path for performing full collection scans.
 * It acquires a shared read lock and delegates the retrieval to the `PrimaryIndexer`.
 * @param collection The name of the collection.
 * @return A vector of deep copies of all documents currently in the primary index.
 */
std::vector<aevum::bson::doc::Document> IndexManager::get_all_documents(
    std::string_view collection) const {
    std::shared_lock<std::shared_mutex> lock(rw_lock_);
    return primary_indexer_.get_all_documents(std::string(collection));
}

/**
 * @brief Retrieves documents by querying a secondary index with a key-value pair.
 * @details This is the primary method for leveraging secondary indexes. It delegates the query to
 * the `SecondaryIndexer` and acquires a shared read lock for concurrent execution.
 * @param collection The collection to search within.
 * @param field The indexed field to query.
 * @param value The value to match for the given field.
 * @return A `std::vector` of matching documents. An empty vector is returned if no matches are
 * found.
 */
std::vector<aevum::bson::doc::Document> IndexManager::get_documents_by_secondary_index(
    std::string_view collection, std::string_view field, std::string_view value) const {
    std::shared_lock<std::shared_mutex> lock(rw_lock_);
    return secondary_indexer_.get_documents_by_secondary_index(
        std::string(collection), std::string(field), std::string(value));
}

/**
 * @brief Adds a document to all relevant indexes (primary and secondary).
 * @details This write operation acquires an exclusive lock. It extracts the document's `_id` and
 * adds it to the `PrimaryIndexer`. It then calls `update_custom_index` on the `SecondaryIndexer`
 * to update all relevant field indexes for the document.
 * @param collection The name of the collection.
 * @param doc The document to be added to the indexes.
 */
void IndexManager::add_document_to_indexes(std::string_view collection,
                                           const aevum::bson::doc::Document &doc) {
    std::string id = extract_id(doc);
    std::string coll_str(collection);

    std::unique_lock<std::shared_mutex> lock(rw_lock_);
    if (!id.empty()) {
        primary_indexer_.add_document_to_primary_index(coll_str, id, doc);
    }
    secondary_indexer_.update_custom_index(coll_str, doc, true);  // `true` for addition
}

/**
 * @brief Removes a document from all relevant indexes (primary and secondary).
 * @details This write operation acquires an exclusive lock. It extracts the document's `_id` to
 * remove it from the `PrimaryIndexer` and then calls `update_custom_index` on the
 * `SecondaryIndexer` with the `add` flag set to `false` to remove it from field indexes.
 * @param collection The name of the collection.
 * @param doc The document to be removed from the indexes.
 */
void IndexManager::remove_document_from_indexes(std::string_view collection,
                                                const aevum::bson::doc::Document &doc) {
    std::string id = extract_id(doc);
    std::string coll_str(collection);

    std::unique_lock<std::shared_mutex> lock(rw_lock_);
    if (!id.empty()) {
        primary_indexer_.remove_document_from_primary_index(coll_str, id);
    }
    secondary_indexer_.update_custom_index(coll_str, doc, false);  // `false` for removal
}

/**
 * @brief Loads all secondary index definitions from persistent storage into the `SecondaryIndexer`.
 * @details This function is critical for database startup. It acquires an exclusive lock on the
 * manager and invokes the `IndexPersistor` to read from the `_indexes` system collection and
 * populate the in-memory index configuration.
 * @return `aevum::util::Status::OK()` on success, or an `IOError` if the persistor fails.
 */
aevum::util::Status IndexManager::load_all_index_definitions() {
    std::unique_lock<std::shared_mutex> lock(rw_lock_);
    aevum::util::log::Logger::debug("IndexManager: Loading all index definitions from persistor.");
    bool success = index_persistor_.load_index_definitions(
        secondary_indexer_.get_all_indexed_fields_mutable());

    if (!success) {
        return aevum::util::Status::IOError("Failed to load index definitions from storage.");
    }
    return aevum::util::Status::OK();
}

/**
 * @brief Persists the current state of secondary index definitions to durable storage.
 * @details This private helper is a write operation that acquires an exclusive lock. It delegates
 * the task of serializing and writing the index metadata to the `IndexPersistor`.
 * @return `aevum::util::Status::OK()` on success, or an `IOError` if persistence fails.
 */
aevum::util::Status IndexManager::persist_index_definitions() {
    // Acquire a write lock to prevent modifications to index definitions during persistence.
    std::unique_lock<std::shared_mutex> lock(rw_lock_);
    aevum::util::log::Logger::debug(
        "IndexManager: Persisting current index definitions to storage.");
    bool success =
        index_persistor_.persist_index_definitions(secondary_indexer_.get_all_indexed_fields());

    if (!success) {
        aevum::util::log::Logger::error("IndexManager: Failed to persist index definitions.");
        return aevum::util::Status::IOError("Failed to persist index definitions.");
    }
    return aevum::util::Status::OK();
}

}  // namespace aevum::db::index

// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file primary_indexer.cpp
 * @brief Implements the `PrimaryIndexer` class for managing high-speed, `_id`-based lookups.
 * @details This file provides the concrete implementations for the `PrimaryIndexer`'s methods.
 * It encapsulates the thread-safe logic for adding, retrieving, updating, and removing
 * documents from the in-memory primary index cache using a reader-writer lock pattern for
 * optimal concurrency.
 */
#include "aevum/db/index/primary_indexer.hpp"

#include <mutex>
#include <shared_mutex>
#include <vector>

namespace aevum::db::index {

/**
 * @brief Retrieves a document from the primary index using its collection and unique ID.
 * @details This function is optimized for high-concurrency read access. It acquires a
 * `std::shared_lock`, allowing multiple threads to perform lookups simultaneously. The lookup
 * proceeds in two stages: first finding the map for the specified collection, then finding the
 * document by its `_id` within that map. If found, a deep copy of the document is returned.
 *
 * @param coll The name of the collection to search within.
 * @param id The unique `_id` string of the document.
 * @return An `std::optional` containing a `aevum::bson::doc::Document` copy if the lookup is
 *         successful. If the collection or the specific document ID is not found, it returns
 *         `std::nullopt`.
 */
std::optional<aevum::bson::doc::Document> PrimaryIndexer::get_document_by_id(
    const std::string &coll, const std::string &id) const {
    // Acquire a shared (read) lock to allow for maximum concurrency during lookups.
    std::shared_lock<std::shared_mutex> lock(primary_index_lock_);

    auto it_coll = id_indexes_.find(coll);
    if (it_coll != id_indexes_.end()) {
        auto it_doc = it_coll->second.find(id);
        if (it_doc != it_coll->second.end()) {
            // A match was found. The Document is returned by value, creating a deep copy
            // and ensuring the caller receives an independent object.
            return it_doc->second;
        }
    }
    // The document or collection was not found in the index.
    return std::nullopt;
}

/**
 * @brief Retrieves all documents currently stored in the primary index for a given collection.
 * @details This function provides a mechanism for performing full collection scans in memory.
 * It acquires a shared read lock and iterates through all entries in the collection's nested map,
 * returning a vector of deep copies of all documents.
 *
 * @param coll The name of the collection.
 * @return A vector of BSON documents. Returns an empty vector if the collection does not exist.
 */
std::vector<aevum::bson::doc::Document> PrimaryIndexer::get_all_documents(
    const std::string &coll) const {
    std::vector<aevum::bson::doc::Document> documents;
    std::shared_lock<std::shared_mutex> lock(primary_index_lock_);

    auto it_coll = id_indexes_.find(coll);
    if (it_coll != id_indexes_.end()) {
        for (const auto &[id, doc] : it_coll->second) {
            documents.push_back(doc);
        }
    }
    return documents;
}

/**
 * @brief Adds or updates a document entry in the primary index.
 * @details This is a write operation that requires exclusive access to the index. It acquires
 * a `std::unique_lock`, serializing this operation and blocking all other readers and writers.
 * The function uses `operator[]` on the nested maps, which conveniently handles both insertion
 * (if the key is new) and update (if the key already exists). The provided document is deep-copied
 * into the index.
 *
 * @param coll The name of the collection.
 * @param id The unique `_id` of the document, which serves as the lookup key.
 * @param doc The `aevum::bson::doc::Document` to be indexed.
 */
void PrimaryIndexer::add_document_to_primary_index(const std::string &coll, const std::string &id,
                                                   const aevum::bson::doc::Document &doc) {
    // Acquire a unique (exclusive) lock to ensure atomic modification of the index.
    std::unique_lock<std::shared_mutex> lock(primary_index_lock_);
    id_indexes_[coll][id] = doc;  // Performs a deep copy and insertion/update.
}

/**
 * @brief Removes a document from the primary index using its unique identifier.
 * @details This is a write operation that requires an exclusive lock to guarantee atomicity.
 * It first finds the map for the specified collection and then erases the document entry
 * corresponding to the given `_id`. If the collection or ID is not found, the operation has no
 * effect.
 *
 * @param coll The name of the collection from which to remove the document.
 * @param id The `_id` of the document to be removed.
 */
void PrimaryIndexer::remove_document_from_primary_index(const std::string &coll,
                                                        const std::string &id) {
    // Acquire a unique (exclusive) lock for safe modification.
    std::unique_lock<std::shared_mutex> lock(primary_index_lock_);
    auto it_coll = id_indexes_.find(coll);
    if (it_coll != id_indexes_.end()) {
        // Erase the document from the inner map if it exists.
        it_coll->second.erase(id);
    }
}

/**
 * @brief Erases all primary index entries associated with a specific collection.
 * @details This is a destructive write operation that requires an exclusive lock. It removes the
 * entire nested map for the given collection, effectively clearing its primary index. This is
 * typically used when a collection is dropped or during a full rebuild.
 *
 * @param coll The name of the collection whose primary index data should be completely cleared.
 */
void PrimaryIndexer::clear_collection_index(const std::string &coll) {
    // Acquire a unique (exclusive) lock to safely remove the entire collection entry.
    std::unique_lock<std::shared_mutex> lock(primary_index_lock_);
    id_indexes_.erase(coll);
}

}  // namespace aevum::db::index

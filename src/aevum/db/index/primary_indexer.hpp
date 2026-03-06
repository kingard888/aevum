// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file primary_indexer.hpp
 * @brief Defines the `PrimaryIndexer` class, responsible for managing the high-speed,
 * in-memory primary key (`_id`) index.
 * @details This header declares a component specialized for the most critical and frequent
 * type of lookup: direct retrieval of a document by its unique `_id`. The `PrimaryIndexer`
 * uses a hash map for near O(1) average-case complexity and is fully thread-safe for
 * high-concurrency environments.
 */
#pragma once

#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "aevum/bson/doc/document.hpp"

namespace aevum::db::index {

/**
 * @class PrimaryIndexer
 * @brief Manages an in-memory, thread-safe hash map for direct `_id`-to-document lookups.
 *
 * @details This class serves as the definitive, high-performance cache for mapping a document's
 * unique identifier (`_id`) to its full BSON representation. It maintains a two-level
 * `unordered_map` structure (`collection_name -> (document_id -> BSON_document)`), providing
 * an efficient mechanism to bypass full collection scans when a document's `_id` is known.
 *
 * To guarantee data integrity and support high-concurrency workloads, access to the index is
 * managed by a `std::shared_mutex`. This allows for numerous, simultaneous, non-blocking
 * read operations (`get_document_by_id`) while ensuring that all write operations
 * (`add`, `remove`, `clear`) are serialized and atomic.
 */
class PrimaryIndexer {
  public:
    /**
     * @brief Constructs a new, empty `PrimaryIndexer`.
     */
    PrimaryIndexer() = default;

    /**
     * @brief Default destructor.
     */
    ~PrimaryIndexer() = default;

    /**
     * @brief Retrieves a deep copy of a document by its collection name and unique `_id`.
     * @details This is the primary query method for this class. It performs a highly concurrent,
     * read-locked lookup in the internal hash map.
     * @param coll The name of the collection in which to search.
     * @param id The unique `_id` string of the document to retrieve.
     * @return An `std::optional<aevum::bson::doc::Document>` containing a deep copy of the
     *         BSON document if a match is found.
     * @return `std::nullopt` if the collection does not exist or if no document with the
     *         specified `_id` is found in the index.
     */
    [[nodiscard]] std::optional<aevum::bson::doc::Document> get_document_by_id(
        const std::string &coll, const std::string &id) const;

    /**
     * @brief Retrieves all documents currently stored in the primary index for a given collection.
     * @param coll The name of the collection.
     * @return A vector of BSON documents. Returns an empty vector if the collection does not exist.
     */
    [[nodiscard]] std::vector<aevum::bson::doc::Document> get_all_documents(
        const std::string &coll) const;

    /**
     * @brief Adds a new document to the primary index or updates an existing one.
     * @details This is a write operation that acquires an exclusive lock. If a document with the
     * given `_id` already exists in the index, its value is overwritten with the new `doc`.
     * If not, a new entry is created. The operation involves a deep copy of the document.
     * @param coll The name of the collection.
     * @param id The unique `_id` of the document, which will serve as the key.
     * @param doc The BSON document to be indexed.
     */
    void add_document_to_primary_index(const std::string &coll, const std::string &id,
                                       const aevum::bson::doc::Document &doc);

    /**
     * @brief Atomically removes a document from the primary index using its `_id`.
     * @details This write operation acquires an exclusive lock. If the specified document is found,
     * it is erased from the index. If not found, the operation has no effect.
     * @param coll The name of the collection.
     * @param id The `_id` of the document to remove.
     */
    void remove_document_from_primary_index(const std::string &coll, const std::string &id);

    /**
     * @brief Completely clears all primary index entries for a specific collection.
     * @details This is a destructive write operation that acquires an exclusive lock. It is
     * typically used when a collection is dropped or during a full index rebuild.
     * @param coll The name of the collection whose primary index is to be cleared.
     */
    void clear_collection_index(const std::string &coll);

  private:
    /**
     * @var id_indexes_
     * @brief The core data structure for the primary index. It is a two-level hash map structured
     * as: `std::unordered_map<CollectionName, std::unordered_map<DocumentID, BsonDocument>>`. This
     * provides efficient, nested lookups by collection and then by ID.
     */
    std::unordered_map<std::string, std::unordered_map<std::string, aevum::bson::doc::Document>>
        id_indexes_;

    /**
     * @var primary_index_lock_
     * @brief A reader-writer mutex that provides thread-safe access to the `id_indexes_` cache.
     * It is marked `mutable` to permit locking within `const` member functions like
     * `get_document_by_id`.
     */
    mutable std::shared_mutex primary_index_lock_;
};

}  // namespace aevum::db::index

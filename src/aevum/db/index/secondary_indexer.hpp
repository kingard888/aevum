// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file secondary_indexer.hpp
 * @brief Defines the `SecondaryIndexer` class, a sophisticated component for managing custom,
 * field-based in-memory indexes.
 * @details This header declares the class responsible for creating, maintaining, and querying
 * secondary indexes. These indexes provide accelerated lookups on non-primary-key fields,
 * significantly improving query performance for common filter criteria. The `SecondaryIndexer`
 * employs a complex, multi-level map structure and is fully thread-safe, using a
 * reader-writer lock to manage concurrent access.
 */
#pragma once

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "aevum/bson/doc/document.hpp"

namespace aevum::db::index {

/**
 * @class SecondaryIndexer
 * @brief Manages a sophisticated, in-memory inverted index for arbitrary document fields.
 *
 * @details This class is the powerhouse of secondary indexing in AevumDB. It maintains a
 * deeply nested data structure (`Collection -> Field -> Stringified Value -> List of Documents`)
 * that functions as an inverted index. This allows for extremely fast retrieval of all documents
 * that contain a specific value for an indexed field.
 *
 * It is engineered for a high-concurrency environment, using a `std::shared_mutex` to allow
 * parallel, non-blocking read operations (queries) while ensuring that write operations
 * (updates, additions, removals) are serialized and atomic.
 */
class SecondaryIndexer {
  public:
    /**
     * @brief Constructs a new, empty `SecondaryIndexer` instance.
     */
    SecondaryIndexer() = default;

    /**
     * @brief Default destructor.
     */
    ~SecondaryIndexer() = default;

    /**
     * @brief Atomically updates all relevant secondary indexes for a given document.
     * @details This is a critical method called upon document insertion, update, or deletion. It
     * acquires an exclusive write lock and inspects the document. For each field in the document
     * that is registered as indexed for its collection, this function updates the inverted index
     * by either adding a reference to the document or removing it.
     *
     * @param coll The name of the collection to which the document belongs.
     * @param doc The BSON document being added to or removed from the collection.
     * @param add A boolean flag indicating the operation type: `true` to add the document to the
     *        indexes, `false` to remove it.
     */
    void update_custom_index(const std::string &coll, const aevum::bson::doc::Document &doc,
                             bool add);

    /**
     * @brief Retrieves all documents that match a specific value in a secondary index.
     * @details This method provides the primary query interface for the secondary index. It
     * performs a highly concurrent, read-locked lookup to find all documents in a given collection
     * that have a specific value for an indexed field.
     *
     * @param coll The name of the collection to search within.
     * @param field The indexed field to query against.
     * @param value The string representation of the value to match.
     * @return A `std::vector` of BSON documents matching the criteria. If no matches are found, or
     *         if the field is not indexed, an empty vector is returned.
     */
    [[nodiscard]] std::vector<aevum::bson::doc::Document> get_documents_by_secondary_index(
        const std::string &coll, const std::string &field, const std::string &value) const;

    /**
     * @brief Performs a thread-safe check to determine if a field is indexed for a collection.
     * @param coll The name of the collection.
     * @param field The name of the field to check.
     * @return `true` if the field is actively indexed for the specified collection, `false`
     * otherwise.
     */
    [[nodiscard]] bool is_field_indexed(const std::string &coll, const std::string &field) const;

    /**
     * @brief Registers a new field to be indexed for a collection.
     * @details This method exclusively modifies the index metadata, marking a field as "indexable".
     * It does not backfill data from existing documents; a full index rebuild is required for that.
     * The operation acquires an exclusive write lock.
     * @param coll The name of the collection.
     * @param field The name of the field to register for indexing.
     */
    void add_indexed_field(const std::string &coll, const std::string &field);

    /**
     * @brief Atomically clears all secondary index entries for a specific collection.
     * @details This is a destructive operation, typically used when a collection is dropped or
     * during a full index rebuild. It acquires an exclusive write lock.
     * @param coll The name of the collection whose secondary indexes should be completely cleared.
     */
    void clear_collection_indexes(const std::string &coll);

    /**
     * @brief Provides read-only, thread-safe access to the map of all registered indexed fields.
     * @details This method is primarily consumed by the `IndexPersistor` to facilitate the saving
     * of index definitions to persistent storage.
     * @return A constant reference to the internal map of collection-to-field mappings.
     */
    [[nodiscard]] const std::unordered_map<std::string, std::unordered_set<std::string>> &
    get_all_indexed_fields() const;

    /**
     * @brief Provides mutable, thread-safe access to the map of all registered indexed fields.
     * @details This is a privileged operation intended for use during system initialization,
     * allowing the `IndexPersistor` to populate the index definitions from storage.
     * Direct modification outside of this context is strongly discouraged.
     * @return A mutable reference to the internal map of collection-to-field mappings.
     */
    [[nodiscard]] std::unordered_map<std::string, std::unordered_set<std::string>> &
    get_all_indexed_fields_mutable();

  private:
    /**
     * @var indexed_fields_
     * @brief Metadata map tracking which fields are indexed per collection.
     * The structure is `collection_name -> {field1, field2, ...}`.
     */
    std::unordered_map<std::string, std::unordered_set<std::string>> indexed_fields_;

    /**
     * @var custom_indexes_
     * @brief The core multi-level inverted index data structure.
     * It maps: `Collection Name -> Field Name -> Stringified Field Value -> Vector of BSON
     * Documents`. This structure allows for rapid lookups based on field values.
     */
    std::unordered_map<
        std::string,
        std::unordered_map<
            std::string, std::unordered_map<std::string, std::vector<aevum::bson::doc::Document>>>>
        custom_indexes_;

    /**
     * @var secondary_index_lock_
     * @brief A reader-writer mutex providing thread-safe, concurrent access to all secondary index
     * structures.
     */
    mutable std::shared_mutex secondary_index_lock_;

    /**
     * @brief A robust internal helper to extract and stringify a field's value from a BSON
     * document.
     * @details This utility is fundamental to the indexing process. It can handle various BSON
     * data types (UTF8 String, Int32, Int64, Double, Bool) and convert them into a canonical
     * string format suitable for use as a key in the `custom_indexes_` map. This normalization
     * ensures consistent indexing across different data types.
     * @param doc The BSON document from which to extract the value.
     * @param field The name of the field whose value is to be extracted and stringified.
     * @return The string representation of the field's value. Returns an empty string if the
     *         field is not found or its type is not supported for indexing.
     */
    [[nodiscard]] std::string get_value_as_string(const aevum::bson::doc::Document &doc,
                                                  const std::string &field) const;

    /**
     * @brief A specialized helper to efficiently extract the `_id` of a document.
     * @details This is used internally during index removal operations to uniquely identify and
     *          locate the specific document to be erased from an index entry's vector.
     * @param doc The BSON document whose `_id` is to be retrieved.
     * @return The `_id` as a string, or an empty string if it cannot be found.
     */
    [[nodiscard]] std::string get_doc_id(const aevum::bson::doc::Document &doc) const;
};

}  // namespace aevum::db::index

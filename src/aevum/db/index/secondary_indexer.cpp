// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file secondary_indexer.cpp
 * @brief Implements the `SecondaryIndexer` class for managing and querying custom, field-based
 * indexes.
 * @details This file provides the concrete implementations for the `SecondaryIndexer`'s methods.
 * It contains the logic for updating the multi-level inverted index structure, handling data
 * type stringification for index keys, and executing queries against the secondary indexes in a
 * thread-safe manner.
 */
#include "aevum/db/index/secondary_indexer.hpp"

#include <algorithm>
#include <bson/bson.h>
#include <mutex>
#include <shared_mutex>

namespace aevum::db::index {

/**
 * @brief Private helper to extract the `_id` field value from a BSON document as a string.
 * @details This function is a specialized wrapper around `get_value_as_string` for the specific
 * purpose of retrieving a document's unique identifier.
 * @param doc The document from which to extract the `_id`.
 * @return A `std::string` containing the `_id`, or an empty string if not found.
 */
std::string SecondaryIndexer::get_doc_id(const aevum::bson::doc::Document &doc) const {
    return get_value_as_string(doc, "_id");
}

/**
 * @brief Extracts the value of a specified field from a BSON document and converts it into a
 * canonical string representation suitable for indexing.
 * @details This is a crucial utility function that enables the indexing of various BSON data types.
 * It iterates through the BSON document to find the specified `field`. If found, it checks the
 * field's type and converts its value into a `std::string`.
 *
 * Supported types and their conversions:
 * - `BSON_TYPE_UTF8`: The raw string value is returned.
 * - `BSON_TYPE_INT32`, `BSON_TYPE_INT64`, `BSON_TYPE_DOUBLE`: The numeric value is converted to its
 *   standard string representation via `std::to_string`.
 * - `BSON_TYPE_BOOL`: The boolean is converted to either "true" or "false".
 *
 * If the field is not found, or if its type is not one of the supported types for indexing (e.g.,
 * an array, a sub-document), an empty string is returned, effectively preventing the field from
 * being indexed for that document.
 *
 * @param doc The BSON document to be inspected.
 * @param field The key of the field whose value is to be extracted.
 * @return A `std::string` containing the canonical representation of the field's value. Returns
 *         an empty string if the field is absent, null, or of an unsupported type.
 */
std::string SecondaryIndexer::get_value_as_string(const aevum::bson::doc::Document &doc,
                                                  const std::string &field) const {
    if (doc.empty() || !doc.get()) return "";

    bson_iter_t iter;
    if (bson_iter_init_find(&iter, doc.get(), field.c_str())) {
        if (BSON_ITER_HOLDS_UTF8(&iter)) {
            uint32_t length;
            return std::string(bson_iter_utf8(&iter, &length), length);
        } else if (BSON_ITER_HOLDS_INT32(&iter)) {
            return std::to_string(bson_iter_int32(&iter));
        } else if (BSON_ITER_HOLDS_INT64(&iter)) {
            return std::to_string(bson_iter_int64(&iter));
        } else if (BSON_ITER_HOLDS_DOUBLE(&iter)) {
            return std::to_string(bson_iter_double(&iter));
        } else if (BSON_ITER_HOLDS_BOOL(&iter)) {
            return bson_iter_bool(&iter) ? "true" : "false";
        }
    }
    return "";
}

/**
 * @brief Adds or removes a document from all applicable secondary indexes within a collection.
 * @details This function is the primary entry point for modifying the secondary index state. It
 * acquires an exclusive write lock to ensure atomic updates. The function first checks if any
 * fields are indexed for the given collection; if not, it returns immediately. It then extracts the
 * document's `_id` and iterates through all registered indexed fields. For each indexed field, it
 * extracts the corresponding value from the document, stringifies it, and then either appends
 * the document to the inverted index list or erases it based on the `add` flag.
 *
 * @param coll The name of the collection being modified.
 * @param doc The document to be added or removed from the indexes.
 * @param add If `true`, the document is added to the index. If `false`, it is removed by finding
 *        and erasing the entry with a matching `_id`.
 */
void SecondaryIndexer::update_custom_index(const std::string &coll,
                                           const aevum::bson::doc::Document &doc, bool add) {
    std::unique_lock<std::shared_mutex> lock(secondary_index_lock_);

    auto it_fields = indexed_fields_.find(coll);
    if (it_fields == indexed_fields_.end()) return;

    std::string doc_id = get_doc_id(doc);
    if (doc_id.empty()) return;

    for (const auto &field : it_fields->second) {
        std::string value_str = get_value_as_string(doc, field);
        if (value_str.empty()) continue;

        auto &doc_vec = custom_indexes_[coll][field][value_str];

        if (add) {
            doc_vec.push_back(doc);
        } else {
            doc_vec.erase(std::remove_if(doc_vec.begin(), doc_vec.end(),
                                         [&](const aevum::bson::doc::Document &d) {
                                             return get_doc_id(d) == doc_id;
                                         }),
                          doc_vec.end());
        }
    }
}

/**
 * @brief Retrieves a list of documents that match a specific key-value pair in a secondary index.
 * @details This function performs a highly concurrent, read-locked lookup. It traverses the
 * multi-level `custom_indexes_` map to locate the vector of documents associated with the
 * specified collection, field, and value.
 * @param coll The name of the collection to search.
 * @param field The indexed field to query.
 * @param value The stringified value to match within the index.
 * @return A `std::vector` of `aevum::bson::doc::Document` copies that match the query. If no
 *         index or value is found, an empty vector is returned.
 */
std::vector<aevum::bson::doc::Document> SecondaryIndexer::get_documents_by_secondary_index(
    const std::string &coll, const std::string &field, const std::string &value) const {
    std::shared_lock<std::shared_mutex> lock(secondary_index_lock_);

    auto it_coll = custom_indexes_.find(coll);
    if (it_coll != custom_indexes_.end()) {
        auto it_field = it_coll->second.find(field);
        if (it_field != it_coll->second.end()) {
            auto it_val = it_field->second.find(value);
            if (it_val != it_field->second.end()) {
                return it_val->second;
            }
        }
    }
    return {};
}

/**
 * @brief Checks if a specific field is registered for indexing within a given collection.
 * @details This function performs a read-locked check against the `indexed_fields_` metadata map.
 * @param coll The collection name.
 * @param field The field name to check.
 * @return `true` if the field is configured for indexing in the collection, `false` otherwise.
 */
bool SecondaryIndexer::is_field_indexed(const std::string &coll, const std::string &field) const {
    std::shared_lock<std::shared_mutex> lock(secondary_index_lock_);
    auto it = indexed_fields_.find(coll);
    if (it != indexed_fields_.end()) {
        return it->second.count(field) > 0;
    }
    return false;
}

/**
 * @brief Registers a new field to be indexed for a specific collection.
 * @details This write operation acquires an exclusive lock to safely modify the `indexed_fields_`
 * map.
 * @param coll The name of the collection.
 * @param field The name of the field to add to the set of indexed fields for that collection.
 */
void SecondaryIndexer::add_indexed_field(const std::string &coll, const std::string &field) {
    std::unique_lock<std::shared_mutex> lock(secondary_index_lock_);
    indexed_fields_[coll].insert(field);
}

/**
 * @brief Completely removes all secondary index data associated with a specific collection.
 * @details This is a destructive write operation that acquires an exclusive lock. It is used when
 * clearing a collection or performing a full index rebuild.
 * @param coll The name of the collection whose secondary indexes will be erased.
 */
void SecondaryIndexer::clear_collection_indexes(const std::string &coll) {
    std::unique_lock<std::shared_mutex> lock(secondary_index_lock_);
    custom_indexes_.erase(coll);
}

/**
 * @brief Provides read-only access to the complete map of registered indexed fields.
 * @details This function is designed for components like `IndexPersistor` that need to read the
 * index configuration for persistence. It is thread-safe via the implicit locking of the
 * underlying data structure through its `const` nature in a concurrent context.
 * @return A constant reference to the `indexed_fields_` map.
 */
const std::unordered_map<std::string, std::unordered_set<std::string>> &
SecondaryIndexer::get_all_indexed_fields() const {
    return indexed_fields_;
}

/**
 * @brief Provides mutable access to the map of registered indexed fields.
 * @details This is a privileged, non-`const` method intended primarily for the `IndexPersistor`
 * to populate the index definitions from storage upon database initialization. Callers are
 * responsible for ensuring external synchronization if this method is used in a concurrent context.
 * @return A mutable reference to the `indexed_fields_` map.
 */
std::unordered_map<std::string, std::unordered_set<std::string>> &
SecondaryIndexer::get_all_indexed_fields_mutable() {
    return indexed_fields_;
}

}  // namespace aevum::db::index

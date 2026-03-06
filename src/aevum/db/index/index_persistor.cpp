// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file index_persistor.cpp
 * @brief Implements the `IndexPersistor` class for handling the storage lifecycle of index
 * definitions.
 * @details This file provides the concrete logic for serializing in-memory index configurations
 * into BSON documents and persisting them in a dedicated system collection within the
 * `WiredTigerStore`. It also handles the reverse process of loading these definitions from storage.
 */
#include "aevum/db/index/index_persistor.hpp"

#include <bson/bson.h>

#include "aevum/bson/json/parser.hpp"
#include "aevum/util/log/logger.hpp"

namespace aevum::db::index {

/**
 * @brief Constructs an `IndexPersistor`, establishing a link to the database's storage layer.
 * @param storage A reference to the active `WiredTigerStore` instance that will be used for all
 *        persistence operations.
 */
IndexPersistor::IndexPersistor(aevum::db::storage::WiredTigerStore &storage) : storage_(storage) {}

/**
 * @brief Serializes and persists the complete set of current index definitions to durable storage.
 * @details This function performs a "snapshot" and "replace" operation. It first completely clears
 * the `_indexes` system collection using `sync_collection` with an empty dataset. This ensures that
 * stale index definitions are removed. It then iterates through the provided `indexed_fields` map.
 * For each `collection`-`field` pair, it constructs a new BSON document of the form
 * `{ "collection": "...", "field": "..." }`. This document is then saved to the `_indexes`
 * collection using a composite key (e.g., "collection_name::field_name") to ensure uniqueness.
 *
 * @param indexed_fields A constant reference to the in-memory map representing the complete state
 *        of all secondary indexes (`collection_name -> {field1, field2, ...}`).
 * @return Returns `true` if the entire operation, including clearing and saving all definitions,
 *         completes without error. Returns `false` if any storage operation fails.
 */
bool IndexPersistor::persist_index_definitions(
    const std::unordered_map<std::string, std::unordered_set<std::string>> &indexed_fields) {
    aevum::util::log::Logger::debug("IndexPersistor: Starting persistence of index definitions.");
    // Atomically clear all previous index definitions to ensure a clean slate.
    auto status = storage_.sync_collection("_indexes", {});
    if (!status.ok()) {
        aevum::util::log::Logger::error(
            "IndexPersistor: Failed to clear old index definitions from storage. Status: " +
            status.to_string());
        return false;
    }
    aevum::util::log::Logger::debug("IndexPersistor: Successfully cleared old index definitions.");

    bool overall_success = true;
    int definition_count = 0;

    for (const auto &[collection, fields] : indexed_fields) {
        for (const auto &field : fields) {
            // Dynamically construct a BSON document for each index metadata entry.
            bson_t *b = bson_new();
            bson_append_utf8(b, "collection", -1, collection.c_str(), -1);
            bson_append_utf8(b, "field", -1, field.c_str(), -1);

            aevum::bson::doc::Document doc(b);

            // Use a composite key to uniquely identify each index definition document.
            std::string meta_id = collection + "::" + field;

            // Persist the newly created index definition document into the system collection.
            auto put_status = storage_.put("_indexes", meta_id, doc);
            if (!put_status.ok()) {
                aevum::util::log::Logger::error(
                    "IndexPersistor: Failed to persist index definition for '" + meta_id +
                    "'. Status: " + put_status.to_string());
                overall_success = false;
            } else {
                definition_count++;
            }
        }
    }

    if (overall_success) {
        aevum::util::log::Logger::info("IndexPersistor: Successfully persisted " +
                                       std::to_string(definition_count) + " index definitions.");
    } else {
        aevum::util::log::Logger::error(
            "IndexPersistor: One or more index definitions failed to persist.");
    }
    return overall_success;
}

/**
 * @brief Loads all index definitions from the `_indexes` system collection in durable storage
 * and populates an in-memory map.
 * @details This function is a core part of the database startup sequence. It retrieves all
 * documents from the `_indexes` collection. For each document, it extracts the `collection` and
 * `field` string values. It then uses these values to populate the provided `indexed_fields` map,
 * effectively reconstructing the in-memory representation of the secondary index configuration.
 *
 * @param indexed_fields A mutable reference to the `SecondaryIndexer`'s map, which will be
 *        populated with the loaded definitions.
 * @return Returns `true` upon successful completion of the loading process. This function currently
 *         always returns `true`, as failures to read individual documents are logged but do not
 *         halt the entire process. A more robust implementation might return `false` on parsing
 * errors.
 */
bool IndexPersistor::load_index_definitions(
    std::unordered_map<std::string, std::unordered_set<std::string>> &indexed_fields) {
    aevum::util::log::Logger::debug("IndexPersistor: Loading index definitions from storage.");
    std::vector<aevum::bson::doc::Document> docs = storage_.load_collection("_indexes");
    int loaded_count = 0;

    for (const auto &doc : docs) {
        if (doc.empty() || !doc.get()) continue;

        bson_iter_t iter;
        std::string collection_name;
        std::string field_name;

        // Safely extract the 'collection' name from the BSON document.
        if (bson_iter_init_find(&iter, doc.get(), "collection") && BSON_ITER_HOLDS_UTF8(&iter)) {
            collection_name = bson_iter_utf8(&iter, nullptr);
        }

        // Safely extract the 'field' name from the BSON document.
        if (bson_iter_init_find(&iter, doc.get(), "field") && BSON_ITER_HOLDS_UTF8(&iter)) {
            field_name = bson_iter_utf8(&iter, nullptr);
        }

        // If both fields were successfully extracted, populate the in-memory map.
        if (!collection_name.empty() && !field_name.empty()) {
            indexed_fields[collection_name].insert(field_name);
            loaded_count++;
        }
    }

    aevum::util::log::Logger::info("IndexPersistor: Loaded " + std::to_string(loaded_count) +
                                   " index definitions from storage.");
    return true;
}

}  // namespace aevum::db::index

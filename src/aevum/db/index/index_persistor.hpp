// Copyright (c) 2026 Ananda Firmansyah.
// Licensed under the AEVUMDB COMMUNITY LICENSE, Version 1.0. See LICENSE file in the root
// directory.

/**
 * @file index_persistor.hpp
 * @brief Defines the `IndexPersistor` class, a dedicated component for serializing and
 * deserializing index definitions to and from persistent storage.
 * @details This header declares a crucial bridge between the in-memory index configuration and the
 * underlying `WiredTigerStore`. The `IndexPersistor` is responsible for ensuring that secondary
 * index metadata survives database restarts, allowing for the correct reconstruction of index
 * structures upon initialization.
 */
#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "aevum/db/storage/wiredtiger_store.hpp"

namespace aevum::db::index {

/**
 * @class IndexPersistor
 * @brief Manages the serialization and deserialization of secondary index metadata to and from the
 * persistent `WiredTigerStore`.
 *
 * @details This class provides a focused abstraction for handling the lifecycle of index
 * definitions. It translates the in-memory representation of which fields are indexed for each
 * collection into a storable format within a dedicated system collection (`_indexes`) in
 * WiredTiger. Conversely, it can read from this system collection to repopulate the in-memory
 * configuration when the database starts.
 */
class IndexPersistor {
  public:
    /**
     * @brief Constructs an `IndexPersistor` instance, linking it to a storage backend.
     * @param storage A reference to the `WiredTigerStore` that will be used for all underlying
     *        read and write operations for index metadata.
     */
    explicit IndexPersistor(aevum::db::storage::WiredTigerStore &storage);

    /**
     * @brief Default destructor.
     */
    ~IndexPersistor() = default;

    /**
     * @brief Persists the provided set of index definitions to the `_indexes` system collection.
     * @details This operation is typically destructive, clearing any pre-existing index
     * definitions in storage before writing the new configuration. It iterates through the provided
     * map, creating a distinct BSON document for each collection-field pair.
     *
     * @param indexed_fields A constant reference to a map where keys are collection names and
     *        values are sets of indexed field names for that collection.
     * @return `true` if all definitions are successfully written to storage, `false` otherwise.
     */
    [[nodiscard]] bool persist_index_definitions(
        const std::unordered_map<std::string, std::unordered_set<std::string>> &indexed_fields);

    /**
     * @brief Loads all index definitions from the `_indexes` system collection in storage.
     * @details This method reads all documents from the `_indexes` table and uses their contents
     * to populate the provided `indexed_fields` map, effectively restoring the state of which
     * fields are indexed for each collection.
     *
     * @param indexed_fields A mutable reference to the map that will be populated with the loaded
     *        index definitions. Any existing content in the map may be cleared or merged.
     * @return `true` if the loading process completes successfully (even if no indexes are found),
     *         `false` if a storage-level error occurs.
     */
    [[nodiscard]] bool load_index_definitions(
        std::unordered_map<std::string, std::unordered_set<std::string>> &indexed_fields);

  private:
    /**
     * @var storage_
     * @brief A reference to the underlying persistent storage engine (WiredTiger), used to
     * perform all physical read/write operations for index metadata.
     */
    aevum::db::storage::WiredTigerStore &storage_;
};

}  // namespace aevum::db::index
